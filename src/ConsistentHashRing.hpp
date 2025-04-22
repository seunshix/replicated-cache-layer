#ifndef CONSISTENT_HASH_RING_HPP
#define CONSISTENT_HASH_RING_HPP

#include <string>
#include <vector>
#include <map>
#include <functional>
#include <stdexcept>


// a simple  consistent-hash ring using std::hash and std::map
// static ring: built once in the constructor. supports virtual nodes

class ConsistentHashRing{
    private:
        std::map<std::size_t, std::string> ring_;
        int virtualNodeCount_;

        // wrapper around std::hash<string>
        std::size_t hashFn(const std::string& s) const{
            return std::hash<std::string>{}(s);
        }

    public:
        ConsistentHashRing(const std::vector<std::string>& nodes, int virtualNodeCount = 100) : virtualNodeCount_(virtualNodeCount){
            if(nodes.empty()){
                throw std::runtime_error("Cannot create hash ring with no nodes");
            }
            // insert virtual nodes into the ring
            for (const auto& node : nodes){
                for (int i = 0; i < virtualNodeCount_; ++i){
                    std::string vnodeId = node + "#" + std::to_string(i);
                    std::size_t hash = hashFn(vnodeId);
                    ring_.emplace(hash, node);
                }
            }
        }
        // given a key (e.g., file path), returns the responsible node ID.
        // throw std::runtime_error if the ring is empty

        std::string getNodeForKey(const std::string& key) const{
            if (ring_.empty()){
                throw std::runtime_error("Hash ring is empty");
            }
            std::size_t h = hashFn(key);
            auto it = ring_.lower_bound(h);

            if (it == ring_.end()){
                it = ring_.begin();
            }
            return it->second;
        }
        void removeNode(const std::string& nodeID){
            for (int i = 0; i < virtualNodeCount_; ++i){
                std::string vNodeId = nodeID + "#" + std::to_string(i);
                std::size_t h = hashFn(vNodeId);
                ring_.erase(h);
            }
        }

        void addNode(const std::string& nodeID){
            for (int i = 0; i < virtualNodeCount_; ++i){
                std::string vnodeId = nodeID + "#" + std::to_string(i);
                std::size_t h = hashFn(vnodeId);
                ring_.emplace(h, nodeID);
            }
        }
};

#endif