#ifndef GM_THEME_H
#define GM_THEME_H
#include <SDL.h>

/* Presentation only. These palettes never change session or request identity. */
typedef struct {
 const char *name,*logo;
 SDL_Color background,chrome,surface,selected,border,ink,muted,accent,warning,error,link,code,user;
} GmTheme;
#define GM_RGB(r,g,b) {r,g,b,255}
static const GmTheme gm_themes[4]={
 {"原默认",NULL,GM_RGB(23,25,30),GM_RGB(18,20,25),GM_RGB(30,35,43),GM_RGB(28,43,40),GM_RGB(63,77,85),GM_RGB(228,230,235),GM_RGB(167,174,184),GM_RGB(115,221,189),GM_RGB(244,190,89),GM_RGB(235,139,139),GM_RGB(113,184,255),GM_RGB(27,31,38),GM_RGB(27,53,47)},
 {"EVA / NERV","skins/nerv.bmp",GM_RGB(18,14,27),GM_RGB(26,18,39),GM_RGB(35,26,49),GM_RGB(52,36,74),GM_RGB(107,83,135),GM_RGB(241,235,250),GM_RGB(186,169,206),GM_RGB(174,248,99),GM_RGB(255,195,98),GM_RGB(255,137,147),GM_RGB(174,248,99),GM_RGB(28,21,40),GM_RGB(51,35,70)},
 {"K-ON!","skins/kon.bmp",GM_RGB(249,242,225),GM_RGB(241,229,207),GM_RGB(255,250,239),GM_RGB(247,225,215),GM_RGB(187,158,131),GM_RGB(65,43,32),GM_RGB(113,86,67),GM_RGB(160,40,68),GM_RGB(131,75,10),GM_RGB(176,34,43),GM_RGB(142,40,75),GM_RGB(240,230,210),GM_RGB(246,227,216)},
 {"Pokémon","skins/pokemon.bmp",GM_RGB(14,30,54),GM_RGB(17,35,63),GM_RGB(25,47,77),GM_RGB(37,60,94),GM_RGB(82,110,145),GM_RGB(248,250,255),GM_RGB(185,201,224),GM_RGB(255,210,64),GM_RGB(255,196,86),GM_RGB(255,135,143),GM_RGB(135,198,255),GM_RGB(18,35,59),GM_RGB(71,35,52)}
};
#undef GM_RGB
typedef struct {int skin,logo,header,footer;} GmAppearance;
static inline GmAppearance gm_appearance_default(void){return (GmAppearance){0,1,1,1};}
static inline int gm_appearance_valid(GmAppearance a){return a.skin>=0&&a.skin<4&&(a.logo==0||a.logo==1)&&(a.header==0||a.header==1)&&(a.footer==0||a.footer==1);}
static inline int gm_content_top(GmAppearance a){return a.header?64:0;}
static inline int gm_content_bottom(GmAppearance a){return a.footer?684:720;}
#endif
