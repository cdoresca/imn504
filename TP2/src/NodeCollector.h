/*
 *	(c) XLim, UMR-CNRS
 *	Authors: G.Gilet
 *
 */

#ifndef _NODE_COLLECTOR_H
#define _NODE_COLLECTOR_H

#include "Node.h"
#include <vector>
// Class Node Collector

class NodeCollector {
public:
    NodeCollector(){};
    virtual ~NodeCollector(){};

    virtual void collect(Node *rootNode) {
        nodes.clear();
        collectRecur(rootNode);
    };
    void collectRecur(Node *node) {
        if (node == NULL)
            return;

        nodes.push_back(node);
        for (std::vector<Node *>::iterator it = node->m_Sons.begin(); it != node->m_Sons.end(); ++it)
            collectRecur(*it);
    }

    std::vector<Node *> nodes;
};

#endif
