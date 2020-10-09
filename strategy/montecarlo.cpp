#include "strategy/montecarlo.hh"
#include "utils/game/visualizer.hh"
#include "strategy/policies.hh"

namespace ares{
    void Montecarlo::init(Reasoner* r,GameAnalyzer* a){
        Strategy::init(r,a);
        pool= ThreadPoolFactroy::get(cfg.mct_threads);
        selPolicy = new SelectionPolicy(*reasoner);
        simPolicy = new SimPolicy(*reasoner);
    }
    
    void Montecarlo::start(const Match& match){
        timer.cancel();
        {
            std::lock_guard<std::mutex> lk(lock);
            std::lock_guard<std::mutex> lkt(tree.lock);
            //Build a new game tree,with the root node beign match.game.init
            stoped = false;
            tree.reset(new Node(match.game->init()->clone(),nullptr));
            order = analyzer->isAZSG(match.game,match.strtClck);
            player = 0;
            current = tree.root->state.get();
        }
        // std::thread([match,this]{(*this)(match,-1);}).detach();
    }
    std::pair<Move*,uint> Montecarlo::operator()(const Match& match,uint seq){
        log("[Montecarlo]") << "Selecting a move\n";
        std::atomic_bool done=false;
        std::future<const Term*> future = timer.reset(done,match);
        
        std::lock_guard<std::mutex> lk(lock);
        if( stoped ) { timer.cancel(); return std::pair<Move*,uint>();}
        if( match.takenAction ) {
            tree.select(match.takenAction); //change the root according to made moves
            current = tree.root->state.get();
        }
        struct search
        {
            search(std::atomic_bool& d,ushort mas, Montecarlo& m, ushort i)
            :done(d),id(mas),mc(m),indx(i)
            {}
            void operator()(){
                while (not done)
                {
                    auto* v = (*mc.selPolicy)(mc.tree.root,done,mc.player,indx);
                    if (id==0 and cfg.debug ) mc.dump();
                    auto value = (*mc.simPolicy)(v->state.get(),done);
                    mc.update(v,value);
                }
            }
            std::atomic_bool& done;
            ushort id;
            Montecarlo& mc;
            ushort indx;
        };
        auto indx = reasoner->roleIndex(match.role->get_name());
        for (size_t i = 0; i < cfg.mct_threads; i++)    
            pool->post(search(done,i,*this,indx));
        
        pool->wait();
        timer.cancel(&done);
        player = (player+1) % ( order.size() ? order.size() : 1);  //switch player according to thier turn
        return std::pair<Move*,uint>(future.get(), seq);
    }

    Montecarlo::Node* SelectionPolicy::operator()(Montecarlo::Node* n,const std::atomic_bool& done,Montecarlo::Ptype p,ushort indx)const{
        while ( (not done )and not reasoner.terminal(*n->state))
        {   
            // log("[Montecarlo]") << "Current node is " << "player " << p  <<"s.\n" ;
            if( auto* a = n->actions.next() ){ //is there an unvisited state reachable from here?
                //expand the node
                auto* child = new Montecarlo::Node(reasoner.next(*n->state,*a),a);
                n->add( child );
                child->parent = n;
                return child;
            }
            else
                n = Montecarlo::bestChild(*n,cfg.uct_c,Montecarlo::plyr(p,indx),INFINITY);
                
        }
        return n;
    }
    std::vector<float> SimPolicy::operator()(const State* state,std::atomic_bool& done)const{
        auto* selectetd = state;
        while ( (not done) and not reasoner.terminal(*state))
        {
            auto a = reasoner.randAction(*state);
            auto* nxt = reasoner.next(*state,*a);
            delete a;
            if( state != selectetd ) delete state;
            state = nxt;
        }
        const auto& roles = reasoner.roles();
        std::vector<float> rewards;
        for (auto&& role : roles)
            rewards.push_back( reasoner.reward( *role, state));
        
        // log("[Montecarlo]") << "Terminal state reached\n";
        // log("[Montecarlo ]\n" + n->state->toString() + "\n");
        // log("[Montecarlo ] Frst player reward\n" + to_string(rewards[Ptype::FRST]) + "\n");
        // log("[Montecarlo ] Scnd player reward\n" + to_string(rewards[Ptype::SCND]) + "\n");
        if( state != selectetd ) delete state;
        return rewards;
    }

    
    

    /**********************************************************************/
    /**
     * Implement the inner struct methods
     */
    /**
     * Delete the subtree rooted here.(except this node.)
     */
    inline void Montecarlo::Node::erase(){   //depth first traversal + deletion.
        for (auto &&child : children)
        {
            child->erase();
            delete child;
        }
    }

    /**
     * Tree methods
     */
    /**
     * Reset the game tree
     */
    inline void Montecarlo::Tree::reset(Montecarlo::Node* root_){
        log("[Montecarlo]") << "Reseting tree\n";
        if( origRoot ){
            origRoot->erase();          //delete the (sub)tree rooted here
            delete origRoot;
        }
        origRoot = root = root_;
    }
    /**
     * make the node that is a child of root and whose action == action the new root.
     * @param action the action taken from the root to one of the children
     */
    inline void Montecarlo::Tree::select(const Action* action){
        std::lock_guard<std::mutex> lk(lock);
        log("[Montecarlo]") << "Changing root to child node with action ";
        std::string sep("");
        for (auto &&a : *action)
        {
            std::cout << sep << a->to_string();
            sep = ",";
        }
        std::cout << "\n";
        
        Node* selected = nullptr;
        auto& children = root->children;
        for (auto &&n : children)
        {
            bool found =true;
            for (size_t i = 0; i < action->size(); i++)
                if( (*n->action)[i] != (*action)[i] ){
                    found = false;
                    // n->erase(); //delete the subtree rooted here
                    // delete n;
                    break;
                }

            if( found ) selected = n;
        }
        if( not selected) {
            log("[Montecarlo]") << "Action Not found. Constructing new root.\n" ;
            //need to construct a new tree with root having state = next(root.state, action);
            selected = new Node(mc.reasoner->next(*root->state,*action),nullptr);
            root->add(selected);
        }
        // delete root; for debuggin/visualization
        selected->parent = nullptr;
        selected->action.reset();
        root = selected;
    }

    /**
     * Timer methods
     */
    inline std::future<const Term*> Montecarlo::Timer::reset(std::atomic_bool& done_,const Match& match){
        std::lock_guard<std::mutex> lk(lock);
        auto duration = (match.plyClck * 1000) - cfg.delta_milli;
        uint cseq = ++seq;
        if( done ) {
            logerr("[Montecarlo::Timer]") << " reset during simulation. Reseting previous simulation.\n";
            *done = true;
        }
        log("[Montecarlo::Timer]") << "Reseting timer to " << duration << " milliseconds\n";
        done = &done_;
        cv.notify_all();        //Cancel previous searches (if any)
        auto cb = [cseq,this,&done_,role(match.role->get_name()),duration]()->const Term *{
            {
                std::unique_lock<std::mutex> lk(lock);
                cv.wait_for(lk,std::chrono::milliseconds(duration),[&]{return (bool)done_;});
            }
            done_ = true;           //If time has expired cancel the search
            log("[Montecarlo::Timer]") << "Waking up for sequence number " << cseq << "\n";
            if( cseq != seq ) return nullptr;
            Node* node;
            auto i = mct.reasoner->roleIndex(role);
            {
                std::lock_guard<std::mutex> lk(mct.tree.lock);
                auto* root =mct.tree.root ;
                if( not root ) return nullptr;
                node = root->children.size() ?  mct.bestChild(*root,0,(Montecarlo::Ptype)i,0) : nullptr;
            }
            if( !node ) return nullptr;
            return (*node->action)[i];
        };
        return std::async( std::launch::async, cb);
    }
    void Montecarlo::Timer::cancel(){
        std::lock_guard<std::mutex> lk(lock);
        if( not done ) return ;

        cv.notify_all();        //Cancel previous searches (if any)
        *done = true;
        done = nullptr;
    }

    void Montecarlo::Timer::cancel(std::atomic_bool* done_){
        std::lock_guard<std::mutex> lk(lock);
        if( done != done_ ) return;
        if( done ) *done = true;
        done = nullptr;
    }
    Montecarlo::~Montecarlo(){
        setPolicies(nullptr,nullptr);
    }

    /**
     * Utilities
     */
    inline void dump_(Montecarlo::Node* root,Montecarlo::Node* n, std::string& treeSt){
        std::lock_guard<std::mutex> lk(n->lock);
        string isRoot = ( n==root ? "true" : "false");
        treeSt += R"#({ "state": ")#" + n->state->toStringHtml() + "\",";
        treeSt += R"#(   "root": )#" + isRoot + ",";
        treeSt += R"#(   "visited": )#" + to_string(n->n) + ",";
        treeSt += R"#(   "values": [)#";
        std::string sep="";
        for (auto &&val: n->values)
        {
            treeSt+= sep + to_string( val);
            sep =",";
        }
        treeSt += R"#(], "ucts":[)#";
        sep="";
        for (uint i=0; i < n->values.size(); i++){
            float uctv = (!n->parent) ? 0 : Montecarlo::uct(*n,cfg.uct_c,(Montecarlo::Ptype)i,INFINITY);
            treeSt += sep +"\"" +to_string(uctv) + "\"";
            sep = ",";
        }
        treeSt += "],";
        // float uctv1 = (!n->parent) ? 0 : Montecarlo::uct(*n,cfg.uct_c,(Montecarlo::Ptype)1,INFINITY);
        // treeSt += R"#(   "uct0": ")#" + to_string(uctv0) + "\",";
        // treeSt += R"#(   "uct1": ")#" + to_string(uctv1) + "\",";
        treeSt += R"#("children" : [)#";
        sep="";
        for (auto &&child : n->children){
            dump_(root, child,treeSt );
            treeSt += ",";
        }
        if( n->children.size() ) treeSt.pop_back();
        treeSt += "]}";
    }
    
    void Montecarlo::dump(std::string){
        std::string treeSt;
        if(tree.root ) dump_(tree.root, tree.origRoot, treeSt);
        else treeSt = "{}";
        Strategy::dump(treeSt);
    }
}