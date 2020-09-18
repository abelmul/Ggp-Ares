#include "strategy/montecarlo.hh"
#include "utils/game/visualizer.hh"
#include "strategy/policies.hh"

namespace ares{

    void Montecarlo::init(){
        selPolicy = new SelectionPolicy(*reasoner);
        simPolicy = new SimPolicy(*reasoner);
    }

    Montecarlo::Node* SelectionPolicy::operator()(Montecarlo::Node* n,const std::atomic_bool& done)const{
        log("[Montecarlo::SelectionPolicy]") << "for node " << n->state->toString() << "\n";
        while ( (not done )and not reasoner.terminal(*n->state))
        {   //TODO IF CONCURRENT, NEED TO SYNCHRONIZE N->ANCTIONS!!!
            if( n->actions ){ //are there unvisited state reachable from here?
                //expand the node
                const auto* a = *n->actions;
                ++n->actions;
                auto* child = new Montecarlo::Node(reasoner.next(*n->state,*a),a);
                n->add( child );
                return child;
            }
            else
                n = Montecarlo::bestChild(*n,2);
        }
        return n;
    }
    float SimPolicy::operator()(Montecarlo::Node* n,ushort indx)const{
        log("[Montecarlo::SimulationPolicy]") << "for node " << n->state->toString() << "\n";

        auto* root = n;
        while (not reasoner.terminal(*n->state))
        {
            auto a = reasoner.randAction(*n->state);
            auto* nxt = reasoner.next(*n->state,*a);
            delete a;
            if( n != root ) delete n;   //then n is not part of the tree.
            n = new Montecarlo::Node(nxt,nullptr);
        }
        auto r = reasoner.reward(*reasoner.roles()[indx],n->state.get());
        if( n != root ) delete n;
        return r;
    }
    void Montecarlo::start(const Match& match){
        timer.cancel();
        {
            std::lock_guard<std::mutex> lk(lock);
            //Build a new game tree,with the root node beign match.game.init
            tree.reset(new Node(match.game->init()->clone(),nullptr));
        }
        // std::thread([match,this]{(*this)(match,-1);}).detach();
    }
    std::pair<move_sptr,uint> Montecarlo::operator()(const Match& match,uint seq){
        std::atomic_bool done=false;
        std::future<ares::cnst_term_sptr> future = timer.reset(done,match);
        
        std::lock_guard<std::mutex> lk(lock);
        if( match.takenAction ) tree.select(match.takenAction);
        while (not done)
        {
            auto* v = (*selPolicy)(tree.root,done);
            if( done ) break;
            auto value = (*simPolicy)(v,reasoner->roleIndex(match.role->get_name()));
            update(v,value);
        }
        timer.cancel();
        return std::pair<move_sptr,uint>(future.get(), seq);
    }

    /**********************************************************************/
    /**
     * Implement the inner struct methods
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
        std::lock_guard<std::mutex> lk(lock);
        log("[Montecarlo]") << "Reseting tree\n";
        if( root ){
            root->erase();
            delete root;
        }
        root = root_;
    }
    /**
     * make the node that is a child of root and whose action == action the new root.
     * @param action the action taken from the root to one of the children
     */
    inline void Montecarlo::Tree::select(const ActionIterator::Action* action){
        static visualizer viz;
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
                    n->erase(); //delete the subtree rooted here
                    delete n;
                    break;
                }

            if( found ) selected = n;
        }
        if( not selected) { log("[Montecarlo]") << "Root not changed" ;return;}
        log("[Montecarlo]") << " New root is \n";
        viz.draw(*selected->state);
        delete root;
        selected->parent = nullptr;
        selected->action.reset();
        root = selected;
    }

    /**
     * Timer methods
     */
    inline std::future<ares::cnst_term_sptr> Montecarlo::Timer::reset(std::atomic_bool& done_,const Match& match){
        std::lock_guard<std::mutex> lk(lock);
        auto duration = match.plyClck - cfg.delta_sec;
        log("[Montecarlo::Timer]") << "Reseting timer to " << duration << "seconds\n";
        uint cseq = ++seq;
        if( done ) *done = true;
        done = &done_;
        cv.notify_all();        //Cancel previous searches (if any)
        auto cb = [cseq,this,&done_,role(match.role->get_name()),duration]{
            {
                std::unique_lock<std::mutex> lk(lock);
                cv.wait_for(lk,std::chrono::seconds(duration),[&]{return (bool)done_;});
            }
            done_ = true;           //If time has expired cancel the search
            log("[Montecarlo::Timer]") << "Waking up for sequence number " << cseq << "\n";
            if( cseq != seq ) return move_sptr();
            Node* node;
            {
                std::lock_guard<std::mutex> lk(mct.tree.lock);
                auto* root =mct.tree.root ;
                node = root->children.size() ?  mct.bestChild(*root,0) : nullptr;
            }
            if( !node ) return move_sptr();
            auto i = mct.reasoner->roleIndex(role);
            return (*node->action)[i];
        };
        return std::async( std::launch::async, cb);
    }
    void Montecarlo::Timer::cancel(){
        std::lock_guard<std::mutex> lk(lock);
        if( done ) *done = true;
        done = nullptr;
    }
    Montecarlo::~Montecarlo(){
        log("[~MonteCarlo]");
    }
}