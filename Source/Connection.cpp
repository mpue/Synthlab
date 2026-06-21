    /*
  ==============================================================================

    Connection.cpp
    Created: 2 Apr 2018 5:18:02pm
    Author:  mpue

  ==============================================================================
*/

#include "Connection.h"

#include <limits>

using juce::PathStrokeType;
using juce::Path;
using juce::Graphics;

Connection::Connection()
{
    linePath = Path();
}

Connection::Connection(Module* source, Pin* a, Module* target, Pin* b) {
	this->source = source;
	this->target = target;
	this->a = a;
	this->b = b;
    linePath = Path();
}

Connection::~Connection()
{
    
    for (std::vector<Pin*>::iterator it = a->getConnections().begin(); it != a->getConnections().end();it++) {
        // (*it)->invalidate();
    }
    for (std::vector<Pin*>::iterator it = b->getConnections().begin(); it != b->getConnections().end();it++) {
        // (*it)->invalidate();
    }
    a->getConnections().clear();
    b->getConnections().clear();
}

Path Connection::getPath() {
    return linePath;
}

void Connection::resized() {

    // Nur gerade Segmente: p1 -> waypoints... -> p2.
    linePath.clear();
    linePath.startNewSubPath ((float) p1.x, (float) p1.y);
    for (const auto& w : waypoints)
        linePath.lineTo ((float) w.x, (float) w.y);
    linePath.lineTo ((float) p2.x, (float) p2.y);

    PathStrokeType wideStroke (3.0f);
    wideStroke.createStrokedPath (linePath, linePath);
}

void Connection::paint(Graphics& g) {
    g.fillPath(linePath);
}

void Connection::setPoints(juce::Point<int> &p1, juce::Point<int> &p2) {
    this->p1 = p1;
    this->p2 = p2;
}

int Connection::findWaypoint (juce::Point<int> pos, float radius) const {
    for (int i = 0; i < (int) waypoints.size(); ++i)
        if (waypoints[i].toFloat().getDistanceFrom (pos.toFloat()) <= radius)
            return i;
    return -1;
}

void Connection::insertWaypoint (juce::Point<int> pos) {
    // Punktliste p1, waypoints..., p2 aufbauen, das naechstgelegene Segment
    // suchen und pos genau dort einfuegen. Segment k (zwischen Punkt k und
    // k+1) entspricht der Einfuege-Position k im waypoints-Vektor.
    std::vector<juce::Point<int>> pts;
    pts.reserve (waypoints.size() + 2);
    pts.push_back (p1);
    for (const auto& w : waypoints) pts.push_back (w);
    pts.push_back (p2);

    int   bestSeg  = 0;
    float bestDist = std::numeric_limits<float>::max();
    for (int i = 0; i < (int) pts.size() - 1; ++i) {
        juce::Line<float> seg (pts[i].toFloat(), pts[i + 1].toFloat());
        juce::Point<float> nearest;
        const float d = seg.getDistanceFromPoint (pos.toFloat(), nearest);
        if (d < bestDist) { bestDist = d; bestSeg = i; }
    }

    waypoints.insert (waypoints.begin() + bestSeg, pos);
}

void Connection::removeWaypoint (int index) {
    if (index >= 0 && index < (int) waypoints.size())
        waypoints.erase (waypoints.begin() + index);
}
     
