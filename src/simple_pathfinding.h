#pragma once
#ifndef CATA_SRC_SIMPLE_PATHFINDING_H
#define CATA_SRC_SIMPLE_PATHFINDING_H

#include <limits>
#include <queue>
#include <vector>

#include "enums.h"
#include "point.h"
#include "point_traits.h"

namespace pf
{

static const int rejected = std::numeric_limits<int>::min();

template<typename Point>
struct node {
    Point pos;
    int dir;
    int priority;

    node( const Point &p, int dir, int priority = 0 ) :
        pos( p ),
        dir( dir ),
        priority( priority ) {}

    // Operator overload required by priority queue interface.
    bool operator< ( const node &n ) const {
        return priority > n.priority;
    }
};

template<typename Point>
struct path {
    std::vector<node<Point>> nodes;
};

/**
 * @param source Starting point of path
 * @param dest End point of path
 * @param max_x Max permissible x coordinate for a point on the path
 * @param max_y Max permissible y coordinate for a point on the path
 * @param estimator BinaryPredicate( node &previous, node *current ) returns
 * integer estimation (smaller - better) for the current node or a negative value
 * if the node is unsuitable.
 * @param reporter void ProgressReporter() called on each step of the algorithm.
 */
template<typename Point, class Offsets, class BinaryPredicate, class ProgressReporter>
path<Point> find_path( const Point &source,
                       const Point &dest,
                       const Point &max,
                       Offsets offsets,
                       BinaryPredicate estimator,
                       ProgressReporter reporter )
{
    static_assert( Point::dimension == 2, "This pathfinding function doesn't work for tripoints" );
    using Traits = point_traits<Point>;
    using Node = node<Point>;

    const auto inbounds = [ max ]( const Point & p ) {
        return Traits::x( p ) >= 0 && Traits::x( p ) < Traits::x( max ) &&
               Traits::y( p ) >= 0 && Traits::y( p ) < Traits::y( max );
    };

    const auto map_index = [ max ]( const Point & p ) {
        return Traits::y( p ) * Traits::x( max ) + Traits::x( p );
    };

    path<Point> res;

    if( source == dest ) {
        return res;
    }

    if( !inbounds( source ) || !inbounds( dest ) ) {
        return res;
    }

    const Node first_node( source, 5, 1000 );

    if( estimator( first_node, nullptr ) == rejected ) {
        return res;
    }

    const size_t map_size = Traits::x( max ) * Traits::y( max );

    std::vector<bool> closed( map_size, false );
    std::vector<int> open( map_size, 0 );
    std::vector<short> dirs( map_size, 0 );
    std::priority_queue<Node, std::vector<Node>> nodes;

    nodes.push( first_node );
    open[map_index( source )] = std::numeric_limits<int>::max();

    // use A* to find the shortest path from (x1,y1) to (x2,y2)
    while( !nodes.empty() ) {
        reporter();

        const Node mn( nodes.top() ); // get the best-looking node

        nodes.pop();
        // mark it visited
        closed[map_index( mn.pos )] = true;

        // if we've reached the end, draw the path and return
        if( mn.pos == dest ) {
            Point p = mn.pos;

            res.nodes.reserve( nodes.size() );

            while( p != source ) {
                const int n = map_index( p );
                const int dir = dirs[n];
                res.nodes.emplace_back( p, dir );
                p += offsets[dir];
            }

            res.nodes.emplace_back( p, -1 );

            return res;
        }

        for( size_t dir = 0; dir < offsets.size(); dir++ ) {
            const Point p = mn.pos + offsets[dir];
            const int n = map_index( p );
            // don't allow:
            // * out of bounds
            // * already traversed tiles
            if( !inbounds( p ) || closed[n] ) {
                continue;
            }

            Node cn( p, dir );
            cn.priority = estimator( cn, &mn );

            if( cn.priority == rejected ) {
                continue;
            }
            // record direction to shortest path
            if( open[n] == 0 || open[n] > cn.priority ) {
                // Note: Only works if the offsets are CW/CCW!
                dirs[n] = ( dir + offsets.size() / 2 ) % offsets.size();
                open[n] = cn.priority;
                nodes.push( cn );
            }
        }
    }

    return res;
}

template<typename Point, class BinaryPredicate>
path<Point> find_path_4dir( const Point &source,
                            const Point &dest,
                            const Point &max,
                            BinaryPredicate estimator )
{
    return find_path( source, dest, max, four_adjacent_offsets, estimator, []() {} );
}

template<typename Point, class BinaryPredicate, class ProgressReporter>
path<Point> find_path_4dir( const Point &source,
                            const Point &dest,
                            const Point &max,
                            BinaryPredicate estimator,
                            ProgressReporter reporter )
{
    return find_path( source, dest, max, four_adjacent_offsets, estimator, reporter );
}

template<typename Point, class BinaryPredicate>
path<Point> find_path_8dir( const Point &source,
                            const Point &dest,
                            const Point &max,
                            BinaryPredicate estimator )
{
    return find_path( source, dest, max, eight_adjacent_offsets, estimator, []() {} );
}

template<typename Point = point>
inline path<Point> straight_path( const Point &source,
                                  int dir,
                                  size_t len )
{
    path<Point> res;

    if( len == 0 ) {
        return res;
    }

    Point p = source;

    res.nodes.reserve( len );

    for( size_t i = 0; i + 1 < len; ++i ) {
        res.nodes.emplace_back( p, dir );

        p += four_adjacent_offsets[dir];
    }

    res.nodes.emplace_back( p, -1 );

    return res;
}

} // namespace pf

#endif // CATA_SRC_SIMPLE_PATHFINDING_H
