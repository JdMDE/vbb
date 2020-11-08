/***************************************************************************
 *   Copyright (C) 2020 by Juan Domingo Esteve                             *
 *   Juan.Domingo@uv.es                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <https://www.gnu.org/licenses/>. *
 ***************************************************************************/
#ifndef PDFSLIDES_H
#define PDFSLIDES_H

#include "config.h"
// All usual includes are already included by config

#include <SDL.h>

#include <poppler-document.h>
#include <poppler-page.h>
#include <poppler-page-renderer.h>

/*! \brief Class to read a PDF document and get its rendering as a SDL surface of the appropriate size
 *         
 * This class is constructed with the configuration file name as argument, since it needs several values
 * that were obtained by the config object from the configuration file.
 * 
 * The PDFSlides object (just one per program) will read the requested file (unless you use the program
 * just as a white blackboard) and will manage the page changes to keep track of the current page and return
 * a SDL surface for it when asked by the drawing object (the Canvas).
 *
 * There are only four Gets for this class (the file name, the current page number, the current page surface and the splash surface) 
 * and no setters, since internal values are filled at construction and not altered later.
*/
class PDFSlides
{
 public:
    /**
     * The default number of dots per inch in a PDF. Used to render the page to the right screen or window resolution.
     */
    const float DefaultDPI=72.0;

    /**
     * The number of slided to advance or go back when Fast Forward or Fast Backward is requested.
     */
    const int   NumSlidesJump=10;
    
    /**
     * Constructor
     * \param cfg A reference to a cfg object full with the data got from the configuration file
     * \param fn The PDF file with the slides (it might be the empty string for no file)
     */
    PDFSlides(Config &cfg,std::string fn);

    /**
     * Destructor
     * It will release the memory booked by the loaded document (if any).
     */
    ~PDFSlides();
 
    /**
     * Gets the name of the pdf file loaded, or the empty string if no file has been loaded
     * \return PDF file name, or empty string for empty blackboard
     */
    std::string GetFileName() { return filename; };
    
    /** 
     * Gets the number of the current page
     * \return The current page number, starting from 0
     */
    int GetCurrentPage() { return current_page; };
    
    
    /**
     * Obtains the SDL surface of the current page, so it can be drawn.
     * \return The SDL surface of the current page of the document (that which has to be shown), or nullptr if no document has been loaded and the program is being used as an empty blackboard.
     */
    SDL_Surface *GetCurrentPageSurface();

    /**
     * Obtains the SDL surface of the splash initial screen so it can be drawn.
     * \return The SDL surface of the splash screen, or nullptr if the configuration has indicated that no splash screen is to be shown.
     */
    SDL_Surface *GetSplashSurface();
    
    /**
     * Function to execute the commands intended for the PDFSlide object
     * \param command The command to be executed
     * \return true if the command has been executed (and therefore, the canvas will have to be updated), false if not.
     */
    bool ExecuteCommand(Config::Commands command);
    
 private:
    poppler::document *InitDoc(std::string fn,bool rot);
    SDL_Surface *GetPageSurface(poppler::document *doc,int pagenum,bool rot);
    /**
     * Advances to the next slide, if possible
     * \return true if the current slide is not the last one, false otherwise. 
     */
    bool GoNext();
    
    /**
     * Goes back to the previous slide, if possible
     * \return true if the current slide is not the first one, false otherwise. 
     */
    bool GoPrev();
    
    /**
     * Advances NumSlidesJump slides, if possible, or to the last one otherwise.
     * \return true if the current slide is not the last one (even if the jump has been of less than NumSlidesJump), and false otherwise. 
     */
    bool GoFF();
    
    /**
     * Goes back NumSlidesJump slides, if possible, or to the first one otherwise.
     * \return true if the current slide is not the first one (even if the jump has been of less than NumSlidesJump), and false otherwise. 
     */
    bool GoFB();
    
    /**
     * Goes to the first slide, unless we are already in it.
     * \return true if the current slide was not the first one, false otherwise
     */
    bool GoFirst();
    
    /**
     * Goes to the last slide, unless we are already in it.
     * \return true if the current slide was not the lest one, false otherwise
     */
    bool GoLast();

    std::string filename;
    poppler::document *slidesdoc,*splashdoc;
    bool default_rot;
    Sint32 sch;
    Sint32 scw;
    bool pdfloaded;
    int current_page;
    SDL_Surface *splash_surface;
};

#endif // PDFSLIDES_H
