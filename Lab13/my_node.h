/*
 *  Custom Node Class
 *  Author: Scott Susanto
 */

#ifndef _MY_NODE_H_
#define _MY_NODE_H_

#include <memory>

using namespace std;

class Node
{
private:
    string nodeid;

    int level;
    shared_ptr<Node> pred;

public:
    Node();
    Node(string nodeid, int level, shared_ptr<Node> pred);

    string get_nodeid();

    int get_level();
    shared_ptr<Node> get_pred();

    void set_level(int level);
    void set_pred(shared_ptr<Node> pred);
};

#endif