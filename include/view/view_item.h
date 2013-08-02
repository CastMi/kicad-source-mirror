/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

/**
 * @file view_item.h
 * @brief VIEW_ITEM class definition.
 */

#ifndef __VIEW_ITEM_H
#define __VIEW_ITEM_H

#include <vector>
#include <math/box2.h>

namespace KiGfx
{
// Forward declarations
class GAL;
class PAINTER;
class VIEW;

/**
 * Class VIEW_ITEM -
 * is an abstract base class for deriving all objects that can be added to a VIEW.
 * It's role is to:
 * - communicte geometry, appearance and visibility updates to the associated dynamic VIEW,
 * - provide a bounding box for redraw area calculation,
 * - (optional) draw the object using the GAL API functions for PAINTER-less implementations.
 * VIEW_ITEM objects are never owned by a VIEW. A single VIEW_ITEM can belong to any number of
 * static VIEWs, but only one dynamic VIEW due to storage of only one VIEW reference.
 */
class VIEW_ITEM
{
public:
    /**
     * Enum ViewUpdateFlags.
     * Defines the how severely the shape/appearance of the item has been changed:
     * - APPEARANCE: shape or layer set of the item have not been affected,
     * only colors or visibility.
     * - GEOMETRY: shape or layer set of the item have changed, VIEW may need to reindex it.
     * - ALL: all flags above */

    enum ViewUpdateFlags {
        APPEARANCE  = 0x1,
        GEOMETRY    = 0x2,
        ALL = 0xff
    };

    VIEW_ITEM() : m_view( NULL ), m_visible( true ), m_highlighted( false ),
            m_groups( NULL ), m_groupsSize( 0 ) {}

    /**
     * Destructor. For dynamic views, removes the item from the view.
     */
    virtual ~VIEW_ITEM()
    {
        ViewRelease();
        delete[] m_groups;
    };

    /**
     * Function ViewBBox()
     * returns the bounding box of the item covering all its layers.
     * @return BOX2I - the current bounding box
     */
    virtual const BOX2I ViewBBox() const = 0;

    /**
     * Function ViewDraw()
     * Draws the parts of the object belonging to layer aLayer.
     * viewDraw() is an alternative way for drawing objects if
     * if there is no PAINTER assigned for the view or if the PAINTER
     * doesn't know how to paint this particular implementation of
     * VIEW_ITEM. The preferred way of drawing is to design an
     * appropriate PAINTER object, the method below is intended only
     * for quick hacks and debugging purposes.
     *
     * @param aLayer: current drawing layer
     * @param aGal: pointer to the GAL device we are drawing on
     * @param aVisibleArea: area (in world space coordinates) that is relevant for drawing. For
     * example, when drawing a bitmap, one can clip the blitting area to aVisibleArea, reducing
     * drawing time.
     */
    virtual void ViewDraw( int aLayer, GAL* aGal, const BOX2I& aVisibleArea ) const { };

    /**
     * Function ViewGetLayers()
     * Returns the all the layers within the VIEW the object is painted on. For instance, a D_PAD
     * spans one or more copper layers and a few technical layers. ViewDraw() or PAINTER::Draw() is
     * repeatedly called for each of the layers returned by ViewGetLayers(), depending on the
     * rendering order.
     * @param aLayers[]: output layer index array
     * @param aCount: number of layer indices in aLayers[]
     */
    virtual void ViewGetLayers( int aLayers[], int& aCount ) const = 0;

    /**
     * Function ViewGetRequiredLayers()
     * Returns the all the layers that are required for an item to be drawn. Eg. there is no point
     * to draw netnames, when the track is not visible - so track layer should be marked as
     * required.
     * @param aLayers[]: output layer index array
     * @param aCount: number of layer indices in aLayers[]
     */
    virtual void ViewGetRequiredLayers( int aLayers[], int& aCount ) const;

    /**
     * Function ViewSetVisible()
     * Sets the item visibility.
     *
     * @param aIsVisible: whether the item is visible (on all layers), or not.
     */
    void ViewSetVisible( bool aIsVisible = true );

    /**
     * Function ViewIsVisible()
     * Returns if the item is visible (or not).
     *
     * @return when true, the item is visible (i.e. to be displayed, not visible in the
     * *current* viewport)
     */
    bool ViewIsVisible() const
    {
        return m_visible;
    }

    /**
     * Function ViewSetHighlighted()
     * Sets the item highlight.
     *
     * @param aIsHighlighted: whether the item is highlighted (on all layers), or not.
     */
    void ViewSetHighlighted( bool aIsHighlighted = true );

    /**
     * Function ViewIsHighlighted()
     * Returns if the item is highlighted (or not).
     *
     * @return when true, the item should be displayed as highlighted.
     */
    bool ViewIsHighlighted() const
    {
        return m_highlighted;
    }

    /**
     * Function ViewGetLOD()
     * Returns the level of detail of the item. A level of detail is the minimal VIEW scale that
     * is sufficient for an item to be shown on a given layer.
     */
    virtual unsigned int ViewGetLOD( int aLayer ) const
    {
        // By default always show the item
        return 0;
    }

    /**
     * Function ViewUpdate()
     * For dynamic VIEWs, informs the associated VIEW that the graphical representation of
     * this item has changed. For static views calling has no effect.
     *
     * @param aUpdateFlags: how much the object has changed
     * @param aForceImmediateRedraw: when true, the VIEW is redrawn immediately,
     *  otherwise, it will be redrawn upon next call of VIEW::Update()
     */
    virtual void ViewUpdate( int aUpdateFlags = ALL, bool aForceImmediateRedraw = false );

    /**
     * Function ViewRelease()
     * Releases the item from an associated dynamic VIEW. For static views calling has no effect.
     */
    void ViewRelease();

protected:
    friend class VIEW;

    /**
     * Function viewAssign()
     * Assigns the item to a given dynamic VIEW. Called internally by the VIEW.
     *
     * @param aView[]: dynamic VIEW instance the item is being added to.
     */

    void viewAssign( VIEW* aView )
    {
        // release the item from a previously assigned dynamic view (if there is any)
        ViewRelease();
        m_view = aView;
        deleteGroups();
    }

    VIEW*   m_view;         ///* Current dynamic view the item is assigned to.
    bool    m_visible;      ///* Are we visible in the current dynamic VIEW.
    bool    m_highlighted;  ///* Should item be drawn as highlighted

private:
    ///* Helper for storing cached items group ids
    typedef std::pair<int, int> GroupPair;

    ///* Indexes of cached GAL display lists corresponding to the item (for every layer it occupies).
    ///* (in the std::pair "first" stores layer number, "second" stores group id).
    GroupPair* m_groups;
    int        m_groupsSize;

    /**
     * Function getGroup()
     * Returns number of the group id for the given layer, or -1 in case it was not cached before.
     *
     * @param aLayer is the layer number for which group id is queried.
     * @return group id or -1 in case there is no group id (ie. item is not cached).
     */
    int getGroup( int aLayer ) const;

    /**
     * Function getAllGroups()
     * Returns all group ids for the item (collected from all layers the item occupies).
     *
     * @return vector of group ids.
     */
    std::vector<int> getAllGroups() const;

    /**
     * Function setGroup()
     * Sets a group id for the item and the layer combination.
     *
     * @param aLayer is the layer numbe.
     * @param aGroup is the group id.
     */
    void setGroup( int aLayer, int aGroup );

    /**
     * Function deleteGroups()
     * Removes all of the stored group ids. Forces recaching of the item.
     */
    void deleteGroups();

    /**
     * Function storesGroups()
     * Returns information if the item uses at least one group id (ie. if it is cached at all).
     *
     * @returns true in case it is cached at least for one layer.
     */
    bool storesGroups() const;
};
} // namespace KiGfx

#endif