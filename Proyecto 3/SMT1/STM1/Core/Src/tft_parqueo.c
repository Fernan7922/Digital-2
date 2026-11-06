/* ILI9341 parking view driven by the global ESP32 byte. Graphics drawn as RGB565 primitives.
 * P1 = bit7, P8 = bit0. No framebuffer, SD, touch or external graphics library.
 */
#include "main.h"
#include "tft_parqueo.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
extern SPI_HandleTypeDef hspi2;
#define W 320
#define H 240
#define WHITE 0xFFFFU
#define BLACK 0x0841U
#define BG 0xEF7DU
#define BAY 0xD69AU
#define YELLOW 0xFDC0U
#define GREEN 0x05C8U
#define RED 0xE8A3U
#define BLUE 0x249FU
/* 0x28: landscape; use 0xE8 to rotate 180 degrees, not to reverse P numbering. */
#define TFT_MADCTL 0x28U
static uint8_t state;
static uint8_t view_valid = 2U;
static uint8_t spi_error;
static void tx(const uint8_t *b, uint16_t n) {
    if (!spi_error && HAL_SPI_Transmit(&hspi2,(uint8_t *)b,n,100U)!=HAL_OK)
        spi_error=1U;
}
static void command(uint8_t c, const uint8_t *data, uint16_t n) {
    HAL_GPIO_WritePin(TFT_CS_GPIO_Port,TFT_CS_Pin,GPIO_PIN_RESET);
    HAL_GPIO_WritePin(TFT_DC_GPIO_Port,TFT_DC_Pin,GPIO_PIN_RESET);
    tx(&c,1);
    HAL_GPIO_WritePin(TFT_DC_GPIO_Port,TFT_DC_Pin,GPIO_PIN_SET);
    if(n) tx(data,n);
    HAL_GPIO_WritePin(TFT_CS_GPIO_Port,TFT_CS_Pin,GPIO_PIN_SET);
}
static void rect(int x,int y,int w,int h,uint16_t color) {
    static uint8_t pixels[640];
    if(x<0){w+=x;x=0;} if(y<0){h+=y;y=0;}
    if(x+w>W)w=W-x;
    if(y+h>H)h=H-y;
    if(w<=0 || h<=0 || spi_error)return;
    uint8_t a[4]={(uint8_t)(x>>8),(uint8_t)x,(uint8_t)((x+w-1)>>8),(uint8_t)(x+w-1)};
    uint8_t b[4]={(uint8_t)(y>>8),(uint8_t)y,(uint8_t)((y+h-1)>>8),(uint8_t)(y+h-1)};
    command(0x2A,a,4); command(0x2B,b,4);
    HAL_GPIO_WritePin(TFT_CS_GPIO_Port,TFT_CS_Pin,GPIO_PIN_RESET);
    HAL_GPIO_WritePin(TFT_DC_GPIO_Port,TFT_DC_Pin,GPIO_PIN_RESET);
    uint8_t c=0x2C; tx(&c,1);
    HAL_GPIO_WritePin(TFT_DC_GPIO_Port,TFT_DC_Pin,GPIO_PIN_SET);
    for(int i=0;i<320;i++){pixels[2*i]=(uint8_t)(color>>8);pixels[2*i+1]=(uint8_t)color;}
    uint32_t count=(uint32_t)w*(uint32_t)h;
    while(count && !spi_error){uint16_t n=(count>320U)?320U:(uint16_t)count;tx(pixels,2U*n);count-=n;}
    HAL_GPIO_WritePin(TFT_CS_GPIO_Port,TFT_CS_Pin,GPIO_PIN_SET);
}
/* Original small 5x7 font, row masks; uppercase and digits only. */
static const uint8_t glyphs[36][7]={
 {14,17,19,21,25,17,14},{4,12,4,4,4,4,14},{14,17,1,2,4,8,31},
 {30,1,1,14,1,1,30},{2,6,10,18,31,2,2},{31,16,16,30,1,1,30},
 {14,16,16,30,17,17,14},{31,1,2,4,8,8,8},{14,17,17,14,17,17,14},
 {14,17,17,15,1,1,14},
 {14,17,17,31,17,17,17},{30,17,17,30,17,17,30},{14,17,16,16,16,17,14},
 {30,17,17,17,17,17,30},{31,16,16,30,16,16,31},{31,16,16,30,16,16,16},
 {14,17,16,23,17,17,15},{17,17,17,31,17,17,17},{14,4,4,4,4,4,14},
 {7,2,2,2,2,18,12},{17,18,20,24,20,18,17},{16,16,16,16,16,16,31},
 {17,27,21,21,17,17,17},{17,25,21,19,17,17,17},{14,17,17,17,17,17,14},
 {30,17,17,30,16,16,16},{14,17,17,17,21,18,13},{30,17,17,30,20,18,17},
 {15,16,16,14,1,1,30},{31,4,4,4,4,4,4},{17,17,17,17,17,17,14},
 {17,17,17,17,17,10,4},{17,17,17,21,21,21,10},{17,17,10,4,10,17,17},
 {17,17,10,4,4,4,4},{31,1,2,4,8,16,31}
};
static void text(int x,int y,const char *s,int scale,uint16_t color) {
    while(*s){char c=*s++;int idx=-1;
        if(c>='0'&&c<='9')idx=c-'0';else if(c>='A'&&c<='Z')idx=c-'A'+10;
        if(idx>=0)for(int row=0;row<7;row++)for(int col=0;col<5;col++)
            if(glyphs[idx][row]&(1U<<(4-col)))rect(x+col*scale,y+row*scale,scale,scale,color);
        x+=6*scale;
    }
}
static void lcd_init(void) {
    spi_error=0;
    HAL_GPIO_WritePin(TFT_CS_GPIO_Port,TFT_CS_Pin,GPIO_PIN_SET);
    HAL_GPIO_WritePin(TFT_RST_GPIO_Port,TFT_RST_Pin,GPIO_PIN_RESET);HAL_Delay(20);
    HAL_GPIO_WritePin(TFT_RST_GPIO_Port,TFT_RST_Pin,GPIO_PIN_SET);HAL_Delay(150);
    command(0x01,NULL,0);HAL_Delay(150);
    const uint8_t power1[]={0x23}, power2[]={0x10}, vcom[]={0x3E,0x28},vcom2[]={0x86};
    const uint8_t format[]={0x55}, orient[]={TFT_MADCTL}, frame[]={0x00,0x18},display[]={0x08,0x82,0x27};
    command(0x28,NULL,0);
    command(0xC0,power1,1);command(0xC1,power2,1);command(0xC5,vcom,2);command(0xC7,vcom2,1);
    command(0x36,orient,1);command(0x3A,format,1);command(0xB1,frame,2);command(0xB6,display,3);
    command(0x20,NULL,0);command(0x11,NULL,0);HAL_Delay(150);
    command(0x29,NULL,0);HAL_Delay(20);
}
static unsigned free_count(uint8_t bits) {
    unsigned free=8;for(unsigned i=0;i<8;i++)if(bits&(1U<<i))free--;return free;
}
static void car(int x,int y,uint16_t body) {
    rect(x+1,y+10,5,12,BLACK);rect(x+28,y+10,5,12,BLACK);
    rect(x+1,y+38,5,12,BLACK);rect(x+28,y+38,5,12,BLACK);
    rect(x+7,y,20,60,BLACK);rect(x+4,y+5,26,50,BLACK);
    rect(x+8,y+2,18,56,body);rect(x+6,y+7,22,46,body);
    rect(x+9,y+15,16,10,0xAE7FU);rect(x+10,y+26,14,14,body);
    rect(x+9,y+41,16,8,0x324BU);
    rect(x+7,y+26,2,14,0x324BU);rect(x+25,y+26,2,14,0x324BU);
    rect(x+8,y+4,5,3,WHITE);rect(x+21,y+4,5,3,WHITE);
    rect(x+8,y+54,4,3,RED);rect(x+22,y+54,4,3,RED);
    rect(x+12,y+8,10,2,WHITE);
}
static void bay(unsigned p,uint8_t occupied) {
    int x=8+(int)(p%4)*51,y=(p<4)?40:140;
    static const uint16_t colors[8]={0xA346,0x4C68,0xAD05,0xFBE0,0xFBE0,0x3296,0x4C68,0xA346};
    rect(x,y,48,86,BAY);rect(x,y,3,86,YELLOW);rect(x+45,y,3,86,YELLOW);
    char name[3]={'P',(char)('1'+p),0};text(x+18,y+3,name,1,BLACK);
    if(occupied)car(x+7,y+14,colors[p]);
    rect(x+13,y+77,22,5,occupied?RED:GREEN);
}
static void number(unsigned n) {
    static const uint8_t segments[9]={0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,0x7F};
    const int pos[7][4]={{244,86,44,7},{288,93,7,37},{288,137,7,37},
                         {244,174,44,7},{237,137,7,37},{237,93,7,37},{244,130,44,7}};
    rect(230,78,73,110,WHITE);
    for(unsigned i=0;i<7;i++)rect(pos[i][0],pos[i][1],pos[i][2],pos[i][3],
                                      (segments[n]&(1U<<i))?BLUE:BG);
}
static void screen(uint8_t next,int full) {
    if(full){rect(0,0,W,H,BG);rect(0,0,W,30,BLACK);text(112,8,"PARQUEO",2,WHITE);
        rect(220,36,94,192,WHITE);text(234,48,"PARQUEOS",1,BLACK);
        text(234,62,"LIBRES",1,BLACK);text(228,198,"DISPONIBLES",1,BLACK);
        text(20,130,"ENTRADA",1,BLACK);}
    for(unsigned i=0;i<8;i++)if(full || ((state^next)&(0x80U>>i)))bay(i,(next&(0x80U>>i))!=0);
    if(full || state!=next)number(free_count(next));
    state=next;

}

/* Called only in foreground, never from I2C callbacks. */
uint8_t TFT_Parqueo_Init(void)
{
    lcd_init();
    view_valid=2U;
    TFT_Parqueo_Actualizar(0U,0U);
    return !spi_error;
}
uint8_t TFT_Parqueo_Error(void) { return spi_error; }
void TFT_Parqueo_Actualizar(uint8_t bits, uint8_t valid)
{
    valid=valid?1U:0U;
    if(spi_error)return;
    if(valid) {
        if(view_valid!=1U || state!=bits) {
            screen(bits,view_valid!=1U);
            if(view_valid!=1U) text(240,214,"ESP32 OK",1,GREEN);
        }
    } else if(view_valid!=0U) {
        screen(0U,1);
        /* Unknown is not FREE: neutral bays, no car, no green indicators. */
        for(unsigned p=0;p<8;p++) {
            int x=8+(int)(p%4)*51,y=(p<4)?40:140;
            rect(x+13,y+77,22,5,0x8410U);
            text(x+12,y+36,"SIN",1,BLACK);
            text(x+9,y+48,"DATO",1,BLACK);
        }
        rect(230,78,73,110,WHITE);
        rect(244,130,44,7,0x8410U);
        text(231,214,"SIN CONEXION",1,RED);
    }
    view_valid=valid;
}
