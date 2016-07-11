/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN Michele Castellana <michele.castellana@cern.ch>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef __SHAPE_COMPOUND_H
#define __SHAPE_COMPOUND_H

#include <geometry/shape.h>
#include <geometry/shape_convex.h>
#include <geometry/polypartition.h>
#include <math/vector2d.h>
#include <math/box2.h>
#include <list>
#include <vector>

class SHAPE_COMPOUND : public SHAPE
{
   public:
      SHAPE_COMPOUND(const std::vector<std::vector<VECTOR2I>> polygons,
            const std::vector<std::vector<VECTOR2I>> holes = std::vector<std::vector<VECTOR2I>>()
            ) : SHAPE( SH_COMPOUND ), is_valid(false)
      {
         std::list<TPPLPoly> input;
         unsigned int i = 0;
         TPPLPoly tmp;
         for( const auto& elem : polygons ) {
            if( elem.empty() )
               continue;
            tmp.Init( elem.size() );
            for( const auto& item : elem ) {
               TPPLPoint to_add = { static_cast<tppl_float>(item.x), static_cast<tppl_float>(item.y) };
               tmp.GetPoints()[i++] = to_add;
            }
            assert(tmp.GetOrientation() == TPPL_CCW );
            input.push_back( tmp );
         }
         for( const auto& elem : holes ) {
            if( elem.empty() )
               continue;
            tmp.Init( elem.size() );
            i = 0;
            for( const auto& item : elem ) {
               TPPLPoint to_add = { static_cast<tppl_float>(item.x), static_cast<tppl_float>(item.y) };
               tmp.GetPoints()[i++] = to_add;
            }
            assert(tmp.GetOrientation() == TPPL_CW );
            tmp.SetHole(true);
            input.push_back( tmp );
         }
         std::list<TPPLPoly> result;
         is_valid = TPPLPartition::Triangulate_EC( &input, &result );
         for( auto& item : result ) {
            SHAPE_CONVEX new_convex;
            for( i = 0; i < item.GetNumPoints(); ++i ) {
               new_convex.Append(item.GetPoints()[i].x, item.GetPoints()[i].y);
            }
            shapes_.push_back( new_convex );
         }
      }

      // Destructor
      ~SHAPE_COMPOUND() {};

      SHAPE_COMPOUND* Clone() const override
      {
         return new SHAPE_COMPOUND( *this );
      }

      const std::string Format() const override
      {
         std::stringstream ss;
         unsigned int i = 0;
         for( const auto& elem : shapes_ ) {
            ss << "SHAPE_CONVEX " << ++i << ":" << std::endl;
            ss << elem.Format() << std::endl;
         }
         return ss.str();
      }

      bool Collide( const SEG& aSeg, int aClearance = 0 ) const
      {
         if(!Valid())
            return true;
         bool ret = false;
         for( auto& item : shapes_ )
         {
            ret = item.Collide(aSeg, aClearance);
            if( ret )
               break;
         }
         return ret;
      };

      const std::vector<SHAPE_CONVEX> Shapes() const
      {
         return shapes_;
      }

      const BOX2I BBox( int aClearance = 0 ) const
      {
         if(!Valid())
            return BBox();
         int x_min = 0;
         int x_max = 0;
         int y_min = 0;
         int y_max = 0;
         for( auto& item : shapes_ ) {
            SHAPE_LINE_CHAIN line = item.Vertices();
            for(int i = 0; i < line.PointCount(); ++i) {
               if( x_min > line.Point(i).x) {
                  x_min = line.Point(i).x;
               }
               if( x_max < line.Point(i).x) {
                  x_max = line.Point(i).x;
               }
               if( y_min > line.Point(i).y) {
                  y_min = line.Point(i).y;
               }
               if( y_max < line.Point(i).y) {
                  y_max = line.Point(i).y;
               }
            }
         }
         return BOX2I( VECTOR2I( x_min, y_min ),
               VECTOR2I( x_max, y_max ));
      }

      int Distance( const SEG& aSeg ) const
      {
         if(!Valid())
            return 0;
         int res = INT_MAX;
         for( const SHAPE_CONVEX& elem : shapes_ )
            res = std::min(res, elem.Distance(aSeg));
         return res;
      }

      bool Valid() const
      {
         return is_valid;
      }

      void Move ( const VECTOR2I& aVector )
      {
         if(!Valid())
            return;
         for( auto& item : shapes_ )
            item.Move( aVector );
      }

      bool operator==( const SHAPE_COMPOUND& aComp ) const
      {
         if(!Valid())
            return false;
         bool res = true;
         for( auto item = aComp.Shapes().begin();
               res && item != aComp.Shapes().end();
               ++item ) {
            if( std::find( shapes_.begin(), shapes_.end(),
                     *item ) == shapes_.end() )
               res = false;
         }
         return res;
      }

      bool operator!=( const SHAPE_COMPOUND& aComp ) const
      {
         return !(*this == aComp);
      }

   private:
      std::vector<SHAPE_CONVEX> shapes_;
      bool is_valid;
};

#endif // __SHAPE_COMPOUND_H
