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

#include "pdfslides.h"

//using namespace std;

PDFSlides::PDFSlides(Config &cfg,std::string fn)
{
 scw=cfg.GetXres();
 sch=cfg.GetYres()-cfg.GetMenuHeight();
 
 if (cfg.GetShowSplash())
 {
  splashdoc=InitDoc(cfg.GetSplashFile(),false);
  splash_surface=GetPageSurface(splashdoc,0,false);
 }
 else
 {
  splashdoc=nullptr;
  splash_surface=nullptr;
 }
 
 filename=fn;
 if (filename=="")
 {
  pdfloaded=false;
  slidesdoc=nullptr;
 }
 else
 {
  pdfloaded=true;
  slidesdoc=InitDoc(filename,false);
 }

 current_page=0;
 default_rot=false;
}

PDFSlides::~PDFSlides()
{
 if (splashdoc!=nullptr)
  delete splashdoc;
 if (slidesdoc!=nullptr)
  delete slidesdoc;
 // splash_surface will be freed by SDL_Quit
}

poppler::document *PDFSlides::InitDoc(std::string fn,bool rot)
{
 if (!poppler::page_renderer::can_render())
 {
  std::cerr << "Renderer compiled without Splash support";
  exit(1);
 }
 
 poppler::document *doc=poppler::document::load_from_file(fn,"","");
 if (doc==nullptr)
 {
  std::cerr << "Error from PDFdoc constructor: loading error. Cannot open file " << fn << std::endl;
  exit(1);
 }
 if (doc->is_locked())
 {
  std::cerr << "Error from PDFdoc constructor: encrypted document";
  exit(1);
 }

 if (doc->pages()<1)
 {
  std::cerr << "Error from PDFdoc constructor: the PDF document has no pages.\n";
  exit(1);
 }
 default_rot=rot;
 
 return doc;
}

bool PDFSlides::GoNext()
{
 if (current_page+1<slidesdoc->pages())
 {
  current_page++;
  return true;
 }
 return false;
}

bool PDFSlides::GoPrev()
{
 if (current_page>0)
 {
  current_page--;
  return true;
 } 
 return false;
}

bool PDFSlides::GoFF()
{ 
 if (current_page+1 < slidesdoc->pages()) 
 { 
  current_page+=NumSlidesJump;
  if (current_page > slidesdoc->pages()-1)
   current_page=slidesdoc->pages()-1;
  return true;
 }
 return false;
}
    
bool PDFSlides::GoFB()
{ 
 if (current_page>0)
 {
  current_page-=NumSlidesJump;
  if (current_page < 0)
   current_page=0;
  return true;
 }
 return false;
}

bool PDFSlides::GoFirst()
{ 
 if (current_page!=0)
 {
  current_page=0;
  return true;
 }
 else
  return false;
}

bool PDFSlides::GoLast()
{ 
 if (current_page!=slidesdoc->pages()-1)
 {
  current_page=slidesdoc->pages()-1;
  return true;
 }
 return false;
}

SDL_Surface *PDFSlides::GetPageSurface(poppler::document *doc,int pagenum,bool rot)
{
 poppler::page_renderer pr;
 pr.set_render_hint(poppler::page_renderer::antialiasing, true);
 pr.set_render_hint(poppler::page_renderer::text_antialiasing, true);
 
 if (pagenum < 0 || pagenum >= doc->pages())
 {
  std::cerr << "Error from get_page_surface: specified page number (" << pagenum << ") out of page count.\n";
  return nullptr;
 }
 poppler::page *p=doc->create_page(pagenum);
 
 Sint32 iw,ih,newiw,newih;
 {
  poppler::image img = pr.render_page(p);
  if (!img.is_valid())
  {
   std::cerr << "Error from get_currentpage_surface: rendering of page " << current_page << " failed.\n";
   exit(1);
  }
  iw=img.width();
  ih=img.height();
  /*
  cerr << "Initial rendering is (" << iw << "," << ih << ")\n";
  cerr << "Screen is (" << scw << "," << sch << ")\n";
  cerr << "Rotation is " << ((rot) ? " true" : " false") << endl;
  */
  float image_ar=float(iw)/float(ih);
  if (!rot)
  {
   // Let's try to adjust width...
   newiw=scw;
   // The new height will be calculated so as to keep the original image aspect ratio...
   newih=int(float(newiw)/image_ar);
   // ... but it might go out of bounds.
   if (newih>sch)
   {
    // In such a case, let's adjust the height
    newih=sch;
    newiw=int(float(newih)*image_ar);
   }
  }
  else
  {
   // If we must show the image rotated, the shown width will be the original height...
   newiw=sch;
   newih=int(float(newiw)*image_ar);
   if (newih>scw)
   {
    newih=scw;
    newiw=int(float(newih)/image_ar);
   }
  }
  //cerr << "New rendering will be " << newiw << "," << newih << " with newip=" << newip << endl;
 }
 
 poppler::rectf inchsize=p->page_rect(poppler::media_box);
 float newdpi=DefaultDPI*( rot ? float(newih)/float(inchsize.height()) : float(newiw)/float(inchsize.width()) );
 /*
 cout << "Original: iw=" << iw << ", ih=" << ih << endl;
 cout << "Transformed: newiw=" << newiw << ", newih=" << newih << endl;
 cout << "Rect: width=" << inchsize.width() << ", height=" << inchsize.height() << endl;
 cout << "rot is " << (rot ? "true" : "false") << endl;
 cout << "New dpi is " << newdpi << endl;
 */
 poppler::image img = pr.render_page(p,newdpi,newdpi,0,0,newiw,newih,(rot) ? poppler::rotate_90 : poppler::rotate_0);

 if (!img.is_valid())
 {
  std::cerr << "Error from get_currentpage_surface: rendering of page " << current_page << " failed.\n";
  exit(1);
 }
 iw=img.width();
 ih=img.height();
 //cerr << "New rendering is (" << iw << "," << ih << ")\n";
 int id=3;
 
 switch (img.format())
 {
  case poppler::image::format_mono: 
	std::cerr << "Error from get_currentpage_surface: The format 'format_mono' is not supported.\n";
	exit(1);
	break;
  case poppler::image::format_rgb24: id=3; break;
  case poppler::image::format_argb32: id=4; break;
  default:
    std::cerr << "Error from get_currentpage_surface: Invalid format. Only 'format_rgb24' and 'format_argb32' are supported\n";
    exit(1);
    break;
 }

 Uint32 rmask,gmask,bmask,amask;
 #if SDL_BYTEORDER == SDL_BIG_ENDIAN
           rmask = 0xff000000;
           gmask = 0x00ff0000;
           bmask = 0x0000ff00;
           amask = 0x000000ff;
 #else
           rmask = 0x000000ff;
           gmask = 0x0000ff00;
           bmask = 0x00ff0000;
           amask = 0xff000000;
 #endif

 SDL_Surface *s=SDL_CreateRGBSurface(SDL_SWSURFACE,iw,ih,8*id,rmask,gmask,bmask,(id==4) ? amask : 0);

 //cout << "Surface created with pointer " << s << endl;
 SDL_PixelFormat *fmt=s->format;

 Uint32 *poi=(Uint32 *)img.data();
 Uint8 *sb=(Uint8 *)s->pixels;
 Uint32 *sb2;
 Uint32 *poi2;
 Uint8 r,g,b,a;
 
 SDL_LockSurface(s);
 
 for (Sint32 row=0;row<ih;row++)
 {
  poi2=poi+row*iw;
  for (Sint32 col=0;col<iw;col++)
  {
   sb2=(Uint32 *)(sb+row*s->pitch+col*fmt->BytesPerPixel);
   b=*poi2 & 0xff;
   g=(*poi2 & 0xff00)>>8;
   r=(*poi2 & 0xff0000)>>16;
   a=(*poi2 & 0xff000000)>>24;
   if (id==3)
    *sb2=SDL_MapRGB(fmt,r,g,b);
   else
    *sb2=SDL_MapRGBA(fmt,r,g,b,a);
   poi2++;
  }
 }
  
 SDL_UnlockSurface(s);
 delete p;
 return(s);   
}

SDL_Surface *PDFSlides::GetCurrentPageSurface()
{ 
 if (pdfloaded)
     return(GetPageSurface(slidesdoc,current_page,default_rot));
 else
     return(nullptr);
}
    
SDL_Surface *PDFSlides::GetSplashSurface()
{
 if (splashdoc==nullptr)
     return(nullptr);
 
 return(GetPageSurface(splashdoc,0,false));
}

bool PDFSlides::ExecuteCommand(Config::Commands command)
{
 switch (command)
 {
  case Config::Next:
    return(GoNext());
    break;
  case Config::Previous:
    return(GoPrev());
    break;
  case Config::FastForward:
    return(GoFF());
    break;
  case Config::FastBackwards:
    return(GoFB());
    break;
  case Config::ToFirstSlide:
    return(GoFirst());
    break;
  case Config::ToLastSlide:
    return(GoLast()); 
    break;
  default: 
    return false;
    break;
 }
}
