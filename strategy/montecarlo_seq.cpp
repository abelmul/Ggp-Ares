#include "strategy/montecarlo_seq.hh"
#include "utils/game/visualizer.hh"
#include "strategy/policies.hh"

namespace ares{
    void MontecarloSeq::init(Reasoner* r){
        Strategy::init(r);
        selPolicy = new SelectionPolicySeq(*reasoner);
        simPolicy = new SimPolicySeq(*reasoner);
    }
    
    void MontecarloSeq::start(const Match& match){
        timer.cancel();
        {
            std::lock_guard<std::mutex> lk(lock);
            std::lock_guard<std::mutex> lkt(tree.lock);
            //Build a new game tree,with the root node beign match.game.init
            stoped = false;
            tree.reset(new Node(match.game->init()->clone(),nullptr));
            current = tree.root->state.get();
        }
        // std::thread([match,this]{(*this)(match,-1);}).detach();
    }
    std::pair<Move*,uint> MontecarloSeq::operator()(const Match& match,uint seq){
        log("[MontecarloSeq]") << "Selecting a move\n";
        std::atomic_bool done=false;
        std::future<const Term*> future = timer.reset(done,match);
        
        std::lock_guard<std::mutex> lk(lock);
        if( stoped ) { timer.cancel(); return std::pair<Move*,uint>();}
        if( match.takenAction ) {
            tree.select(match.takenAction); //change the root according to made moves
            current = tree.root->state.get();
        }
        while (not done)
        {
            auto* v = (*selPolicy)(tree.root,done);
            dump();
            if( done ) break;
            auto value = (*simPolicy)(v,reasoner->roleIndex(match.role->get_name()),done);
            update(v,value);
        }
        timer.cancel(&done);
        return std::pair<Move*,uint>(future.get(), seq);
    }

    MontecarloSeq::Node* SelectionPolicySeq::operator()(MontecarloSeq::Node* n,const std::atomic_bool& done)const{
        while ( (not done )and not reasoner.terminal(*n->state))
        {   
            if( n->actions ){ //is there an unvisited state reachable from here?
                //expand the node
                const auto* a = *n->actions;
                ++n->actions;
                auto* child = new MontecarloSeq::Node(reasoner.next(*n->state,*a),a);
                n->add( child );
                child->parent = n;
                return child;
            }
            else
                n = MontecarloSeq::bestChild(*n,cfg.uct_c,INFINITY);
        }
        return n;
    }
    float SimPolicySeq::operator()(MontecarloSeq::Node* n,ushort indx,std::atomic_bool& done)const{

        auto* root = n;
        while ( (not done) and not reasoner.terminal(*n->state))
        {
            auto a = reasoner.randAction(*n->state);
            auto* nxt = reasoner.next(*n->state,*a);
            delete a;
            if( n != root ) delete n;   //then n is not part of the tree.
            n = new MontecarloSeq::Node(nxt,nullptr);
        }
        auto r = reasoner.reward(*reasoner.roles()[indx],n->state.get());
        if( n != root ) delete n;
        return r;
    }

    
    

    /**********************************************************************/
    /**
     * Implement the inner struct methods
     */
    /**
     * Delete the subtree rooted here.(except this node.)
     */
    inline void MontecarloSeq::Node::erase(){   //depth first traversal + deletion.
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
    inline void MontecarloSeq::Tree::reset(MontecarloSeq::Node* root_){
        log("[MontecarloSeq]") << "Reseting tree\n";
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
    inline void MontecarloSeq::Tree::select(const Action* action){
        std::lock_guard<std::mutex> lk(lock);
        log("[MontecarloSeq]") << "Changing root to child node with action ";
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
            log("[MontecarloSeq]") << "Action Not found. Constructing new root.\n" ;
            //need to construct a new tree with root having state = next(root.state, action);
            selected = new Node(mc.reasoner->next(*root->state,*action),nullptr);
            root->add(selected);
        }
        log("[MontecarloSeq]") << " New root is \n";
        // delete root; for debuggin/visualization
        selected->parent = nullptr;
        selected->action.reset();
        root = selected;
    }

    /**
     * Timer methods
     */
    inline std::future<const Term*> MontecarloSeq::Timer::reset(std::atomic_bool& done_,const Match& match){
        std::lock_guard<std::mutex> lk(lock);
        auto duration = (match.plyClck * 1000) - cfg.delta_sec;
        uint cseq = ++seq;
        if( done ) {
            logerr("[MontecarloSeq::Timer]") << " reset during simulation. Reseting previous simulation.\n";
            *done = true;
        }
        log("[MontecarloSeq::Timer]") << "Reseting timer to " << duration << " milliseconds\n";
        done = &done_;
        cv.notify_all();        //Cancel previous searches (if any)
        auto cb = [cseq,this,&done_,role(match.role->get_name()),duration]()->const Term *{
            {
                std::unique_lock<std::mutex> lk(lock);
                cv.wait_for(lk,std::chrono::milliseconds(duration),[&]{return (bool)done_;});
            }
            done_ = true;           //If time has expired cancel the search
            log("[MontecarloSeq::Timer]") << "Waking up for sequence number " << cseq << "\n";
            if( cseq != seq ) return nullptr;
            Node* node;
            {
                std::lock_guard<std::mutex> lk(mct.tree.lock);
                auto* root =mct.tree.root ;
                if( not root ) return nullptr;
                node = root->children.size() ?  mct.bestChild(*root,0) : nullptr;
            }
            if( !node ) return nullptr;
            auto i = mct.reasoner->roleIndex(role);
            return (*node->action)[i];
        };
        return std::async( std::launch::async, cb);
    }
    void MontecarloSeq::Timer::cancel(){
        std::lock_guard<std::mutex> lk(lock);
        if( not done ) return ;

        cv.notify_all();        //Cancel previous searches (if any)
        *done = true;
        done = nullptr;
    }

    void MontecarloSeq::Timer::cancel(std::atomic_bool* done_){
        std::lock_guard<std::mutex> lk(lock);
        if( done != done_ ) return;
        if( done ) *done = true;
        done = nullptr;
    }
    MontecarloSeq::~MontecarloSeq(){
        setPolicies(nullptr,nullptr);
    }

    /**
     * Utilities
     */
    inline void dump_(MontecarloSeq::Node* root,MontecarloSeq::Node* n, std::string& treeSt){
        string isRoot = ( n==root ? "true" : "false");
        treeSt += R"#({ "state": ")#" + n->state->toStringHtml() + "\",";
        treeSt += R"#(   "root": )#" + isRoot + ",";
        treeSt += R"#(   "visited": )#" + to_string(n->n) + ",";
        float uctv = (!n->parent) ? 0 : MontecarloSeq::uct(*n,cfg.uct_c,INFINITY);
        treeSt += R"#(   "uct": ")#" + to_string(uctv) + "\",";
        treeSt += R"#("children" : [)#";
        std::string sep="";
        for (auto &&child : n->children){
            dump_(root, child,treeSt );
            treeSt += ",";
        }
        if( n->children.size() ) treeSt.pop_back();
        treeSt += "]}";
    }
    
    void MontecarloSeq::dump(std::string){
        std::string treeSt;
        if(tree.root ) dump_(tree.root, tree.origRoot, treeSt);
        else treeSt = "{}";
        Strategy::dump(treeSt);
    }
}