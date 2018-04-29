    /*
  ==============================================================================

    Connection.cpp
    Created: 2 Apr 2018 5:18:02pm
    Author:  mpue

  ==============================================================================
*/

#include "Connection.h"

Connection::Connection()
{
    linePath = new Path();
}

Connection::Connection(Module* source, Pin* a, Module* target, Pin* b) {
	this->source = source;
	this->target = target;
	this->a = a;
	this->b = b;
    linePath = new Path();
}

Connection::~Connection()
{
    a->connections.clear();
    b->connections.clear();
    
    
    for (std::vector<Pin*>::iterator it = a->connections.begin(); it != a->connections.end();it++) {
        (*it)->invalidate();
    }
    for (std::vector<Pin*>::iterator it = b->connections.begin(); it != b->connections.end();it++) {
        (*it)->invalidate();
    }
    
    linePath = nullptr;
     
}

Path* Connection::getPath() {
    return linePath;
}

void Connection::resized() {
    
    linePath->clear();
    linePath->startNewSubPath (p1.x, p1.y);

    linePath->cubicTo (p1.x , p1.y ,
                       p1.x + (p2.x - p1.x) * 0.3f, p1.y ,
                       p1.x + (p2.x - p1.x) * 0.5f, p1.y + (p2.y - p1.y) * 0.5f);
    linePath->cubicTo ( p1.x + (p2.x - p1.x) * 0.5f, p1.y + (p2.y - p1.y) * 0.5f,
                       p2.x - (p2.x - p1.x) * 0.3, p2.y ,
                       p2.x,p2.y  );
    PathStrokeType wideStroke (2.0f);
    wideStroke.createStrokedPath (*linePath, *linePath);
    
}

void Connection::paint(Graphics& g) {
    if (linePath != nullptr)
        g.fillPath(*linePath);
}

void Connection::setPoints(Point<int> &p1, Point<int> &p2) {
    this->p1 = p1;
    this->p2 = p2;
}
     
