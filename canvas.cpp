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
#include "canvas.h"

//using namespace std;

Canvas::Canvas(Config &cfg)
{ 
 last_saved=0;
 if (SDL_Init(SDL_INIT_VIDEO)<0)
 {
  std::cerr << "Error initializing SDL.\n";
  exit(1);
 }
 const SDL_VideoInfo *inf=SDL_GetVideoInfo();
 
 int flags=SDL_SWSURFACE;
 if (!cfg.GetInWin())
 {
  flags|=SDL_FULLSCREEN;
  scw=inf->current_w;
  sch=inf->current_h;
  cfg.SetRes(scw,sch);
 }
 else
 {
  scw=cfg.GetXres();
  sch=cfg.GetYres();
  //window=SDL_CreateWindow("VBB",SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, scw, sch, SDL_WINDOW_RESIZABLE);  
 }
 menu_height=cfg.GetMenuHeight();

 if ((c=SDL_SetVideoMode(scw,sch,inf->vfmt->BitsPerPixel,flags))==nullptr)
 {
  std::cerr << "Error creating main SDL surface. Exiting.\n";
  SDL_Quit();
  exit(1);
 }

 // This code is just to set the window name, so that external programs can locate it for sending events, share it, etc. 
 SDL_SysWMinfo myinfo;
 SDL_VERSION(&myinfo.version);
 if (SDL_GetWMInfo(&myinfo))
 {
  if (cfg.GetInWin())
   XStoreName(myinfo.info.x11.display,myinfo.info.x11.wmwindow,"VirtualBlackboard");
  else
   XStoreName(myinfo.info.x11.display,myinfo.info.x11.fswindow,"VirtualBlackboard");
 
 }

 if ((buf=SDL_CreateRGBSurface(SDL_SWSURFACE,scw,sch-menu_height,c->format->BitsPerPixel,
                               c->format->Rmask,c->format->Gmask,c->format->Bmask,c->format->Amask))==nullptr)
 {
  std::cerr << "Error creating secondary SDL surface. Exiting.\n";
  SDL_Quit();
  exit(1);
 }
 
 lc[Red].r      =0xFF; lc[Red].g=      0x00; lc[Red].b=      0x00; lc[Red].unused      =0x00;
 lc[Green].r    =0x00; lc[Green].g=    0xFF; lc[Green].b=    0x00; lc[Green].unused    =0x00;
 lc[Blue].r     =0x00; lc[Blue].g=     0x00; lc[Blue].b=     0xFF; lc[Blue].unused     =0x00;
 lc[Black].r    =0x00; lc[Black].g=    0x00; lc[Black].b=    0x00; lc[Black].unused    =0x00;
 lc[White].r    =0xFF; lc[White].g=    0xFF; lc[White].b=    0xFF; lc[White].unused    =0x00;
 lc[PaleBlue].r =0x00; lc[PaleBlue].g= 0x00; lc[PaleBlue].b= 0x80; lc[PaleBlue].unused =0x00;
 lc[Yellow].r   =0x00; lc[Yellow].g=   0xFF; lc[Yellow].b=   0xFF; lc[Yellow].unused   =0x00;
 lc[Orange].r   =0xFF; lc[Orange].g=   0x80; lc[Orange].b=   0x80; lc[Orange].unused   =0x00;

 line_draw_col=SDL_MapRGB(c->format,lc[Black].r,lc[Black].g,lc[Black].b);
 line_draw_index=Black;
 line_erase_col=SDL_MapRGB(c->format,lc[White].r,lc[White].g,lc[White].b);

 if (TTF_Init()<0)
 {
  std::cerr << "Error opening TTF engine. Exiting.\n";
  SDL_Quit();
  exit(1);
 }
 if ((tf=TTF_OpenFont(cfg.GetFont().c_str(),cfg.GetFontsize()))==nullptr)
 {
  std::cerr << "Unable to open font " << cfg.GetFont() << ". It does not exists or it is not readable. Exiting.\n";
  SDL_Quit();
  exit(1);
 }
 
 drawstate=Drawing;
 tracing=false;
 line_width=2;
 er_size=2*cfg.GetEraserSize();
 
 SDL_FillRect(buf,nullptr,SDL_MapRGB(buf->format,lc[White].r,lc[White].g,lc[White].b));
 
 save_message=cfg.GetSaveMessage();
 errorsave_message=cfg.GetErrorSaveMessage();
 
 unsigned char *p=(unsigned char *)buf->pixels+(buf->pitch*menu_height);
 // We store the bytes that make a background pixel, to be used later by merge. 
 // 8 is the maximum, probably all formats use up to 4, but just in case...
 // Anyway, merge will only check just as much as needed
 for (int i=0;i<8;i++)
  vback[i]=*(p+i);
 
 // This is not real. It will be changed by SetMenu.
 num_menuitems=1;
 
 // This is not real. They will be changed when pen starts tracing.
 x0=y0=0;
 
 // This call initializes the variables used to draw the line characteristics selection box
 InitLC();
}

void Canvas::ShowSplash(SDL_Surface *splash_surface)
{
 if (splash_surface==nullptr)
  return;

 SDL_Rect r;
 r.x=(splash_surface->w>scw) ? 0 : (scw-splash_surface->w)/2;
 r.y=(splash_surface->h>sch) ? 0 : (sch-splash_surface->h)/2;
 r.w=(splash_surface->w>scw) ? scw : splash_surface->w;
 r.h=(splash_surface->h>sch) ? sch : splash_surface->h;
 SDL_BlitSurface(splash_surface,nullptr,c,&r);
 
 SDL_Event ev;

 Update(Slide);
 do
 {
  SDL_PollEvent(&ev);
 } 
 while ((ev.type!=SDL_KEYDOWN) && (ev.type!=SDL_MOUSEBUTTONDOWN));
 SDL_FreeSurface(splash_surface);
}

void Canvas::Show(SDL_Surface *s)
{
 SDL_Rect r;

 if (s!=nullptr)
 {
  r.x=(s->w>scw) ? 0 : ((scw-s->w)/2);
  r.y=(s->h>(sch-menu_height-1)) ? menu_height : menu_height+((sch-menu_height-1-s->h)/2);
  r.w=(s->w>scw) ? scw : s->w;
  r.h=(s->h>(sch-menu_height-1)) ? (sch-menu_height-1) : s->h;
  SDL_BlitSurface(s,nullptr,c,&r);
  SDL_FreeSurface(s);
 }
 else
  Erase(Slide);
}

void Canvas::Update(UpdatableObjects what)
{
 if ((what == Slide) || (what == Both))
  SDL_UpdateRect(c,0,0,scw,sch);
 if ((what == Buffer) || (what == Both))
  SDL_UpdateRect(buf,0,0,scw,sch-menu_height);
}

void Canvas::Erase(UpdatableObjects what)
{
 SDL_Rect r;
 r.x=0;
 r.y=menu_height;
 r.w=scw;
 r.h=sch-menu_height;
 if ((what == Slide) || (what == Both))
  SDL_FillRect(c,&r,SDL_MapRGB(c->format,lc[White].r,lc[White].g,lc[White].b));
 if ((what == Buffer) || (what == Both))
  SDL_FillRect(buf,&r,SDL_MapRGB(buf->format,lc[White].r,lc[White].g,lc[White].b));
}

// Remember: GetPosCode is called by main only in the event of SDL_MOUSEBUTTONDOWN
Config::Commands Canvas::GetPosCode(int x,int y,bool &for_canvas)
{
 if (y>menu_height)
 {
  tracing=true;
  return Config::NoCommand;
 }
 else
 {
  if (x<menu_height)
   return Config::DrawErase;
  
  int ret=((x-menu_height)*num_menuitems)/(scw-menu_height);
  Config::Commands retcom;
  
  if ( ret+Config::FirstMenuCommand<=Config::LastMenuCommand )
  {
   retcom= Config::Commands(int(Config::FirstMenuCommand) + ret);
   for_canvas=((retcom!=Config::Next) && (retcom!=Config::Previous));
   return(retcom);
  }
  else
   return(Config::NoCommand);
 }
}

void Canvas::DrawConfirmBoxAndWait(const std::string &fn,const std::string &message)
{
 SDL_Rect r;

 std::string complete_message=message;
 complete_message.replace(message.find_first_of("%s"),fn.size(),fn);
 int w,h;
 SDL_Surface *t=TTF_RenderUTF8_Solid(tf,complete_message.c_str(),lc[Black]);
 TTF_SizeUTF8(tf,complete_message.c_str(),&w,&h);

 r.x=(scw-w-20)/2;
 r.y=menu_height+(sch-menu_height-h-50)/2;
 r.w=w+20;
 r.h=h+50;
 Drawrect(r);

 r.x+=10;
 r.y+=10;
 r.w=w;
 r.h=h;
 SDL_BlitSurface(t,nullptr,c,&r);

 r.x=(scw-30)/2;
 r.y=menu_height+(sch-menu_height)/2+7;
 r.w=30;
 r.h=20;
 Drawrect(r);

 SDL_FreeSurface(t);
 t=TTF_RenderUTF8_Solid(tf,"OK",lc[Black]);
 TTF_SizeUTF8(tf,"OK",&w,&h);
 SDL_Rect r1;
 r1.x=(scw-w)/2;
 r1.y=r.y+2;
 r1.w=w;
 r1.h=h;
 SDL_BlitSurface(t,nullptr,c,&r1);

 Update(Slide);

 SDL_Event ev;
 bool quit=false;
 do
 {
  SDL_PollEvent(&ev);
  if ((ev.type==SDL_MOUSEBUTTONDOWN) && (Inside(ev.button.x,ev.button.y,r)))
   quit=true;
 } 
 while (!quit);
}

/**
 * This draw each of the menu esntries. To be cleaned and compacted.
 */
void Canvas::TextWithHighlight(const std::string &s,int row,int col,int d,int h)
{
 std::string s1,s2,s3;
 unsigned int i=0;
 while ((s[i]!='(') && (i<s.size()))
  i++;
 if (i>=s.size())
 {
  std::cerr << "Error in text_with_highlight, first string. Exiting.\n";
  SDL_Quit();
  exit(1);
 }
 s1=s.substr(0,i);
 if (i<s.size()-1)
  i++;
 s2=s.substr(i,1);
 if (i<s.size()-2)
  i+=2;
 s3=s.substr(i,s.size()-i);
 
 int w1,h1,w2,h2,w3,h3;
 SDL_Surface *sa=TTF_RenderUTF8_Solid(tf,s1.c_str(),lc[White]);
 TTF_SizeUTF8(tf,s1.c_str(),&w1,&h1);
 SDL_Surface *sb=TTF_RenderUTF8_Solid(tf,s2.c_str(),lc[Green]);
 TTF_SizeUTF8(tf,s2.c_str(),&w2,&h2);
 SDL_Surface *sc=TTF_RenderUTF8_Solid(tf,s3.c_str(),lc[White]);
 TTF_SizeUTF8(tf,s3.c_str(),&w3,&h3);

 SDL_Rect rt;
 rt.x=col+2;
 rt.y=row;
 rt.w=w1;
 rt.h=h1;
 SDL_BlitSurface(sa,nullptr,c,&rt);
 rt.x=col+w1+2;
 rt.y=row;
 rt.w=w2;
 rt.h=h2;
 SDL_BlitSurface(sb,nullptr,c,&rt);
 rt.x=col+w1+w2+2;
 rt.y=row;
 rt.w=w3;
 rt.h=h3;
 SDL_BlitSurface(sc,nullptr,c,&rt);

 SDL_FreeSurface(sa);
 SDL_FreeSurface(sb);
 SDL_FreeSurface(sc);
}

void Canvas::SetMenu(const std::vector<std::string> &mitems)
{
 num_menuitems=mitems.size();
 
 SDL_Rect r;
 r.x=r.y=0;
 r.w=scw;
 r.h=menu_height;
 SDL_FillRect(c,&r,SDL_MapRGB(c->format,lc[White].r,lc[White].g,lc[White].b));

 DrawmodeSetcolor();

 int item_width=(scw-menu_height)/num_menuitems;
 for (unsigned int i=0;i<mitems.size();i++)
 {
  r.x=menu_height+(item_width*i);
  r.y=0;
  r.h=menu_height;
  r.w=item_width-1;
  SDL_FillRect(c,&r,SDL_MapRGB(c->format,lc[Black].r,lc[Black].g,lc[Black].b));
  TextWithHighlight(mitems[i],r.y+1,r.x+1,item_width,menu_height);
 }
 SDL_UpdateRect(c,0,0,scw,menu_height);
}

/**
 * This is to change the mode-square in the upper-left corner (that one which shows if we are drawing (green) or erasing (red))
 */
void Canvas::DrawmodeSetcolor(void)
{
 SDL_Rect r;
 r.x=1;
 r.y=1;
 r.w=menu_height-2;
 r.h=menu_height-2;
 int d_col=(drawstate==Drawing) ? Green : Red;
 SDL_FillRect(c,&r,SDL_MapRGB(c->format,lc[d_col].r,lc[d_col].g,lc[d_col].b));
 SDL_UpdateRect(c,r.x,r.y,r.w,r.h);
}

/**
 * This is to change the small square in the upper-left corner (inside the mode-square) that signals if we are 
 * tracing (black) or not (red or green, accoding to the current mode)
 */
void Canvas::TracingSetcolor(void)
{
 SDL_Rect r;
 r.x=menu_height/4;
 r.y=menu_height/4;
 r.w=menu_height/2;
 r.h=menu_height/2;
 int d_col=(tracing) ? Black : ((drawstate==Drawing) ? Green : Red);
 SDL_FillRect(c,&r,SDL_MapRGB(c->format,lc[d_col].r,lc[d_col].g,lc[d_col].b));
 SDL_UpdateRect(c,r.x,r.y,r.w,r.h);
}

void Canvas::Drawrect(SDL_Rect r)
{
 SDL_FillRect(c,&r,SDL_MapRGB(c->format,lc[Black].r,lc[Black].g,lc[Black].b));
 r.x++;
 r.y++;
 r.w-=2;
 r.h-=2;
 SDL_FillRect(c,&r,line_erase_col);
}

/**
 * This is a boring and complex procedure that initializes all the graphical objects 
 * that are used to draw the line color and line width selection box. Try to simplify it, if possible...
 */
void Canvas::InitLC()
{
 // The side of each color square. It depends on the screen width, and everything else is calculated according to this proportion
 sqside=scw/24;
 
 // The point of the canvas at which the upper-left corner of the selection box. The heigth is checked to verify that it does
 // not hide the upper menu and it is al least 20 pixels behind it.
 xoff=2*sqside;
 yoff=((2*sqside)>=menu_height+20) ? (2*sqside) : menu_height+20;
 
 // Width and height of the selection box. The width if fixed due to the arrangment we have chosen (exactly two columns)
 // The height depends on the number of colors we have to choose from (NumCols)
 rw=4*sqside;
 rh=int((float(NumCols)/2.0)+0.5)*sqside+int(1.2*float(sqside));
 
 // With all these values, the rectangle for the choice box is set:
 choicerect.x=xoff;
 choicerect.y=yoff;
 choicerect.w=rw;
 choicerect.h=rh;

 // The rectangles in which colors will be shown. They are arranged in two columns of as many rows as needed.
 // The color box is smaller than the square that contains it so color are sepparated and a black rectangle
 // can be shown surrounding the chosen color
 for (int i=0;i<NumCols;i++)
 {
  colrect[i].x=xoff+(i%2)*sqside+3;
  colrect[i].y=yoff+(i/2)*sqside+3;
  colrect[i].w=colrect[i].h=sqside-6;
 }
 
 // The rectangles that will contain the lines of different widths to choose from. All are of the same dimensions,
 // sufficient to accomodate the thickest line.
 for (int i=0;i<MaxLWidth;i++)
 {
  linerect[i].x=xoff+2*sqside+4;
  linerect[i].y=yoff+(sqside/4)+(MaxLWidth+18)*i+2;
  linerect[i].w=2*sqside-6;
  linerect[i].h=MaxLWidth+18;
 }
 
 // The rectangles that represent the lines with different thickness. Since lines are drawn horizontally,
 // width is the same for all and thickness (which is really the height of the rectangle) changes.
 for (int i=0;i<MaxLWidth;i++)
 {
  lines[i].x=xoff+2*sqside+6;
  lines[i].y=yoff+(sqside/4)+(MaxLWidth+18)*i+((MaxLWidth+18)/2)-((i+1)/2);
  lines[i].w=2*sqside-10;
  lines[i].h=i+1;
 }
 
 // The rectangle that will act as confirmation box to contain the "OK" text.
 ok.x=xoff+2;
 ok.y=yoff+((NumCols/2)*sqside)+4;
 ok.w=2*sqside-4;
 ok.h=sqside;
 
 // The OK text is rendered and its width and height are stored
 
 text=TTF_RenderUTF8_Solid(tf,"OK",lc[Black]);
 {
  int tw,th;
  TTF_SizeUTF8(tf,"OK",&tw,&th);
  oktext.x=ok.x+(ok.w/2)-(tw/2);
  oktext.y=ok.y+(ok.h/2)-(th/2);
  oktext.w=tw;
  oktext.h=th;
 }
 
 // The rectangle that will be drawn in black (but empty) around the selected color
 // Its location (x,y) cannot be set now since it will change according the the value of line_draw_index (the user's choice) but width and height are always the same
 defcol.w=defcol.h=sqside;
 
 // The rectangle that will be drawn in black (but empty) around the selected line with thickness.
 // Its y-coordinate cannot be set now since it will change according to the value of line_width (the user's choice) but x, width and height are always the same
 defline.x=xoff+2*sqside+4;
 defline.w=2*sqside-6;
 defline.h=MaxLWidth+18;
}

void Canvas::ChangeLineCharac(void)
{
 bool redraw=true;
 SDL_Event ev;
 
 do
 {
  // The first time, everything is drawn...
  if (redraw)
  {
   // The complete rectangle that contains the choice box is redraw
   Drawrect(choicerect);

   // Then, the rectangle around the chosen color (defined by line_draw_index)
   // Only the things that change for this rectangle (the location (x,y)) are modified
   defcol.x=xoff+(line_draw_index%2)*sqside;
   defcol.y=yoff+(line_draw_index/2)*sqside;
   Drawrect(defcol);

   // Then, the rectangle around the chosen line (defined by line_width)
   // Again, only the thing that changes is modified
   defline.y=yoff+(sqside/4)+((MaxLWidth+18)*(line_width-1))+2;
   Drawrect(defline);
   
   // Then, the colored squares are filled, each with its color
   for (int i=0;i<NumCols;i++)
    SDL_FillRect(c,&colrect[i],SDL_MapRGB(c->format,lc[i].r,lc[i].g,lc[i].b));
   
   // The lines with different widths are black filled rectangles
   for (int i=0;i<MaxLWidth;i++)
    SDL_FillRect(c,&lines[i],SDL_MapRGB(c->format,lc[Black].r,lc[Black].g,lc[Black].b));
   
   // And finally the rectangle with the OK for confirmation...
   Drawrect(ok);
   // ...together with its content
   SDL_BlitSurface(text,nullptr,c,&oktext);

   // and the whole choice box is updated.
   SDL_UpdateRect(c,xoff,yoff,rw,rh); 
 
   //cout << "Redraw done.\n"; char ch; cin >> ch;
  }
  
  // Now, let's look at the mouse click to know if we should redraw or not
  // Not in principle, unless the user clicks on a color box or on a width line box
  redraw=false;
  // Only events of mouse click inside the choice box are considered
  if (SDL_PollEvent(&ev) && (ev.type==SDL_MOUSEBUTTONDOWN) && Inside(ev.button.x,ev.button.y,choicerect))
  {
   // The user has done his/her choice. That's all for this function (and it is the only way to leave it)
   if (Inside(ev.button.x,ev.button.y,ok))
    return;
   
   // Let's check if the click is inside a color box...
   int i=0;
   while ((i<NumCols) && (!Inside(ev.button.x,ev.button.y,colrect[i])))
    i++;
   // If it is, we will take note of the new color (its index and the real color) and the box will have to be redrawn to reflect the new choice
   if (i<NumCols)
   {
    line_draw_index=i;
    line_draw_col=SDL_MapRGB(c->format,lc[i].r,lc[i].g,lc[i].b);
    redraw=true;
   }
   else // If not a color box, let's see if it is a line width box. In this case, redraw, too.
   { 
    i=0;
    while ((i<MaxLWidth) && (!Inside(ev.button.x,ev.button.y,linerect[i])))
     i++;
    if (i<MaxLWidth)
    {
     line_width=i+1;
     redraw=true;
    }
   }
  }
 } 
 while (true); // This is an infinite loop. No way to go out except by cliking on OK
}

void Canvas::WritePnm(std::ofstream &f)
{
 f << "P6\n" << scw << " " << sch-menu_height << "\n255\n";

 SDL_PixelFormat *fmt=c->format;
 Uint8 bpp=fmt->BytesPerPixel;
 
 Uint8 *p8;
 Uint16 *p16;
 Uint32 *p32,co;
 SDL_Color *cs;
 unsigned char cc[3];
 //cerr << "Saving image of " << scw << "x" << sch-menu_height << " with " << int(bpp) << " bytes per pixel.\n";
 switch(bpp)
 {
  case 1: p8=(Uint8 *)c->pixels+(scw*menu_height);
	  for (int i=scw*menu_height;i<scw*sch;i++,p8++)
	  {
	   cs=fmt->palette->colors+(*p8);
           cc[0]=cs->r;
	   cc[1]=cs->g;
	   cc[2]=cs->b;
	   f.write(reinterpret_cast<char const *>(cc),3);
	  }
	  break;
  case 2: p16=(Uint16 *)c->pixels+(scw*menu_height);
	  for (int i=scw*menu_height;i<scw*sch;i++,p16++)
	  {
	   co=(Uint32)((*p16) & (fmt->Rmask));
	   co>>=(fmt->Rshift);
	   co<<=(fmt->Rloss);
	   cc[0]=(unsigned char)co;
	   co=(Uint32)((*p16) & (fmt->Gmask));
	   co>>=(fmt->Gshift);
	   co<<=(fmt->Gloss);
	   cc[1]=(unsigned char)co;
	   co=(Uint32)((*p16) & (fmt->Bmask));
	   co>>=(fmt->Bshift);
	   co<<=(fmt->Bloss);
	   cc[2]=(unsigned char)co;
	   f.write(reinterpret_cast<char const *>(cc),3);
	  }
  case 3:
  case 4: p32=(Uint32 *)c->pixels+(scw*menu_height);
	  for (int i=scw*menu_height;i<scw*sch;i++,p32++)
	  {
	   co=(Uint32)((*p32) & (fmt->Rmask));
	   co>>=(fmt->Rshift);
	   co<<=(fmt->Rloss);
	   cc[0]=(unsigned char)co;
	   co=(Uint32)((*p32) & (fmt->Gmask));
	   co>>=(fmt->Gshift);
	   co<<=(fmt->Gloss);
	   cc[1]=(unsigned char)co;
	   co=(Uint32)((*p32) & (fmt->Bmask));
	   co>>=(fmt->Bshift);
	   co<<=(fmt->Bloss);
	   cc[2]=(unsigned char)co;
	   f.write(reinterpret_cast<char const *>(cc),3);
	  }
	  break;
   default:
        std::cerr << "Error: Blackboard image has an abnormal number of bits per pixel. Exiting.\n";
	    SDL_Quit();
	    exit(1);
 }
}

void Canvas::SaveBlackboard()
{
 char n[3];
 n[0]=char(int('0')+(last_saved/10));
 n[1]=char(int('0')+(last_saved%10));
 n[2]='\0';
 std::string fn="saved_"+std::string(n)+".pnm";

 std::ofstream f;
 f.open(fn.c_str());
 if (!f.is_open())
 {
  DrawConfirmBoxAndWait(fn,errorsave_message);
  return;
 }
 WritePnm(f);
 f.close();
 
 last_saved++;
 if (last_saved>99)
  last_saved=0;
 
 DrawConfirmBoxAndWait(fn,save_message);
}

void Canvas::Drawline(int x1,int y1)
{
 int dx=abs(x1-x0);
 int dy=abs(y1-y0);

 // Too short lines are not drawn. We'll wait until the user has moved mouse a little bit more.
 if ((dx+dy)<MinLDis)
  return;

 int sx=((x1-x0)>0) ? 1 : -1;
 int sy=((y1-y0)>0) ? 1 : -1;

 int x=x0;
 int y=y0;

 int steep=0,e,i,fy,fx;
 SDL_Rect around;
 around.w=around.h=(drawstate==Drawing) ? line_width : er_size;
 Uint32 color=(drawstate==Drawing) ? line_draw_col : line_erase_col;
 if (dy>dx)
 {
  steep=1;
  std::swap(x,y);
  std::swap(dx,dy);
  std::swap(sx,sy);
 }
 e=2*dy-dx;
 for (i=0;i<dx;i++)
 {
  fy=(steep) ? x : y;
  fx=(steep) ? y : x;
  around.x=fx-(line_width/2);
  around.y=fy-(line_width/2);
  SDL_FillRect(c,&around,color);
  SDL_FillRect(buf,&around,color);
  SDL_UpdateRect(c,around.x,around.y,around.w,around.h);
  while (e>=0)
  {
   y+=sy;
   e-=2*dx;
  }
  x+=sx;
  e+=2*dy;
 }

 x0=x1;
 y0=y1;
}

void Canvas::Merge(void)
{
 unsigned char *p=(unsigned char *)buf->pixels+(buf->pitch*menu_height);
 unsigned char *q=(unsigned char *)c->pixels+(c->pitch*menu_height);
 // Bytes per pixel, to know how much we must increment the pointer
 int inc=(buf->pitch/buf->w);
 unsigned char *lim=p+(buf->pitch*(buf->h-menu_height));
 int i;
 int np=0;
 while (p<lim)
 {
  i=0;
  while ((*(p+i)==vback[i]) && (i<inc))
   i++;
  if (i<inc)
  {
   memcpy(q,p,inc);
   np++;
  }
  p+=inc;
  q+=inc;
 }
}

void Canvas::ExecuteCommand(Config::Commands command,SDL_Surface *cs)
{
 switch (command)
 {
  case Config::DrawErase:
        SetTracing(false);
        ToggleDrawmode();
        break;
  case Config::LineCharac:
        ChangeLineCharac();
        Show(cs);
        Merge();
        Update(Both);
        break;
  case Config::EraseAll:
        Erase(Both);
        Update(Both);
        break;
  case Config::EraseSlide:
        Erase(Slide);
        Merge();
        Update(Both);
        break;
  case Config::EraseBlackb:
        Erase(Both);
        Show(cs);
        Merge();
        Update(Both);
        break;
  case Config::SaveBlackb:
        SaveBlackboard();
        Show(cs);
        Merge();
        Update(Both);
        break;
  case Config::Quit:
        break;
  case Config::NoCommand:
      break;
  default:
      break;
 }
}

void Canvas::Prepare(Config &cfg,SDL_Surface *sp,SDL_Surface *sl)
{
 if ( cfg.GetShowSplash() )
 {
  ShowSplash(sp);
  Erase(Slide);
 }
 
 SetMenu(cfg.GetMenuItems());
 
 Show(sl);
 
 Update(Both);
}
