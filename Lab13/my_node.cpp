/*
 *  Custom Node Class
 *  Author: Scott Susanto
 */

#include "my_node.h"

#include <iostream>

Node::Node()
{
    this->nodeid = "";
    this->level = 0;
    this->pred = NULL;
}

Node::Node(string nodeid, int level, shared_ptr<Node> pred)
{
    this->nodeid = nodeid;
    this->level = level;
    this->pred = pred;
}

string Node::get_nodeid()
{
    return nodeid;
}

int Node::get_level()
{
    return level;
}

shared_ptr<Node> Node::get_pred()
{
    return pred;
}

void Node::set_level(int level)
{
    this->level = level;
}

void Node::set_pred(shared_ptr<Node> pred)
{
    this->pred = pred;
}