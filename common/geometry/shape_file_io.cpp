/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * Copyright (C) 2016 CERN
 * @author Michele Castellana <michele.castellana@cern.ch>
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

#include <string>
#include <cassert>

#include <geometry/shape.h>
#include <geometry/shape_file_io.h>

SHAPE_FILE_IO::SHAPE_FILE_IO( const std::string& aFilename, SHAPE_FILE_IO::IO_MODE aMode )
{
    m_groupActive = false;
    m_mode = aMode;

    if( !aFilename.empty() )
    {
        switch( aMode )
        {
            case IOM_READ:
               m_file.open( aFilename.c_str(), std::fstream::in | std::fstream::binary );
               break;
            case IOM_WRITE:
               m_file.open( aFilename.c_str(), std::fstream::out | std::fstream::binary );
               break;
            case IOM_APPEND:
               m_file.open( aFilename.c_str(), std::fstream::app | std::fstream::binary );
               break;
            default:
                return;
        }
    }
}

SHAPE_FILE_IO::~SHAPE_FILE_IO()
{
    if( !m_file.good() )
        return;

    if( m_groupActive && m_mode != IOM_READ )
        m_file << "endgroup" << std::endl;

    m_file.close();
}


SHAPE* SHAPE_FILE_IO::Read()
{
 /*   char tmp[1024];

    do {

        if (fscanf(m_file, "%s", tmp) != 1)
            return NULL;

        if( !strcmp( tmp, "shape" )
            break;
    }

    int type;

    SHAPE *rv = NULL;

    fscanf(m_file,"%d %s", &type, tmp);

    printf("create shape %d\n", type);

    switch(type)
    {
        case SHAPE::LINE_CHAIN:
            rv = new SHAPE_LINE_CHAIN;
            break;
    }

    if(!rv)
        return NULL;

    rv.Parse ( )

    fprintf(m_file,"shape %d %s %s\n", aShape->Type(), aName.c_str(), sh.c_str() );
*/
    assert( false );
    return NULL;
}


void SHAPE_FILE_IO::BeginGroup( const std::string& aName )
{
    assert( m_mode != IOM_READ );

    if( !m_file.good() )
        return;

    m_file << "group " << aName << std::endl;
    m_groupActive = true;
}


void SHAPE_FILE_IO::EndGroup()
{
    assert( m_mode != IOM_READ );

    if( !m_file.good() || !m_groupActive )
        return;

    m_file << "endgroup" << std::endl;
    m_groupActive = false;
}


void SHAPE_FILE_IO::Write( const SHAPE* aShape, const std::string& aName )
{
    assert( m_mode != IOM_READ );

    if( !m_file.good() )
        return;

    if( !m_groupActive )
        m_file << "group default" << std::endl;

    std::string sh = aShape->Format();

    m_file << "shape "
       << aShape->Type() << " "
       << aName << " "
       << sh << std::endl;
    m_file.flush();
}
