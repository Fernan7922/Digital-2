/* Independent ILI9341 / UART test. Graphics drawn as RGB565 primitives.
 * P1 = bit7, P8 = bit0. No framebuffer, SD, touch or external graphics library.
 */
#include "main.h"
#include "tft_demo.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
extern SPI_HandleTypeDef hspi2;
extern UART_HandleTypeDef huart2;
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
static uint8_t color_view;
static uint8_t spi_error;
static uint8_t rx_byte;
#define RX_SIZE 128U
static volatile uint8_t rx_ring[RX_SIZE];
static volatile uint16_t rx_head, rx_tail;
static volatile uint8_t rx_overflow, rx_restart;
static char line[48];
static uint8_t line_len, line_bad;
static void serial(const char *s) {
    (void)HAL_UART_Transmit(&huart2,(uint8_t *)s,(uint16_t)strlen(s),1000U);
}
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
    color_view=0;
}
static void report(void) {
    char bits[9],msg[120];for(unsigned i=0;i<8;i++)bits[i]=(state&(0x80U>>i))?'1':'0';bits[8]=0;
    snprintf(msg,sizeof msg,"\r\nP1-P8: %s | HEX: 0x%02X | Ocupados: %u | Disponibles: %u\r\n",
             bits,(unsigned)state,8U-free_count(state),free_count(state));serial(msg);
    for(unsigned i=0;i<8;i++){snprintf(msg,sizeof msg,"P%u: %s\r\n",i+1,(state&(0x80U>>i))?"OCUPADO":"LIBRE");serial(msg);}
    if(spi_error)serial("ERROR SPI: revisa conexion/configuracion; usa reiniciar para reintentar.\r\n");
    else serial("Datos enviados a TFT. Comprueba la imagen fisicamente.\r\n");
}
static void help(void) {
    serial("\r\nPRUEBA TFT ILI9341 - 8 PARQUEOS\r\n"
           "0..8: cantidad ocupada, empezando en P1. Reemplaza estado anterior.\r\n"
           "estado 10101001: P1, P3, P5, P8 ocupados (izquierda = P1).\r\n"
           "ocupar 5 / liberar 5: cambiar solo P5.\r\n"
           "ver: reporte | ayuda o ?: comandos | redibujar: refrescar TFT\r\n"
           "reiniciar: reiniciar TFT conservando estado.\r\n"
           "colores: bandas roja, verde, azul; redibujar vuelve al parqueo.\r\n"
           "Escribe un comando y ENTER; espera 'Comando>' antes del siguiente.\r\n");
}
/* Returns 1 only for a valid exact eight-bit representation. */
static int parse_bits(const char *s,uint8_t *out) {
    if(strlen(s)!=8U)return 0;
    uint8_t v=0;for(unsigned i=0;i<8;i++){if(s[i]!='0'&&s[i]!='1')return 0;v=(uint8_t)((v<<1)|(s[i]-'0'));}
    *out=v;return 1;
}
static uint8_t count_mask(unsigned n) {return n==0?0:(uint8_t)(0xFFU<<(8U-n));}
static void process(char *s) {
    while(*s==' '||*s=='\t')s++;
    size_t n=strlen(s);while(n && (s[n-1]==' '||s[n-1]=='\t'))s[--n]=0;
    for(size_t i=0;i<n;i++)s[i]=(char)tolower((unsigned char)s[i]);
    uint8_t next=state;
    if(n==1 && s[0]>='0' && s[0]<='8')next=count_mask((unsigned)(s[0]-'0'));
    else if(!strncmp(s,"estado ",7)){const char *p=s+7;while(*p==' ')p++;if(!parse_bits(p,&next))goto invalid;}
    else if(!strncmp(s,"ocupar ",7)||!strncmp(s,"liberar ",8)){
        int set=(s[0]=='o');const char *p=s+(set?7:8);while(*p==' ')p++;
        if(p[0]<'1'||p[0]>'8'||p[1]!=0)goto invalid;
        uint8_t mask=(uint8_t)(0x80U>>(p[0]-'1'));
        next=set?(uint8_t)(state|mask):(uint8_t)(state&~mask);
    }
    else if(!strcmp(s,"ayuda")||!strcmp(s,"?")){help();return;}
    else if(!strcmp(s,"ver")){report();return;}
    else if(!strcmp(s,"redibujar")){screen(state,1);report();return;}
    else if(!strcmp(s,"reiniciar")){lcd_init();screen(state,1);report();return;}
    else if(!strcmp(s,"colores")){
        color_view=1;
        rect(0,0,107,240,0xF800);rect(107,0,106,240,0x07E0);rect(213,0,107,240,0x001F);
        serial("Bandas de izquierda a derecha: ROJO, VERDE, AZUL. Usa redibujar.\r\n");return;
    }
    else goto invalid;
    /* Redraw changed bays; full redraw only after the color test. */
    screen(next,color_view);report();return;
invalid:
    serial("Comando invalido; estado sin cambios. Escribe ayuda.\r\n");
}
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *uart) {
    if(uart->Instance!=USART2)return;
    uint16_t next=(uint16_t)((rx_head+1U)%RX_SIZE);
    if(next==rx_tail)rx_overflow=1;else{rx_ring[rx_head]=rx_byte;rx_head=next;}
    if(HAL_UART_Receive_IT(&huart2,&rx_byte,1U)!=HAL_OK)rx_restart=1;
}
void HAL_UART_ErrorCallback(UART_HandleTypeDef *uart) {
    if(uart->Instance==USART2){rx_restart=1;rx_overflow=1;}
}
/* The generated interrupt file has no USART2 handler in this polling project.
 * It is defined here. Do not add a second definition in stm32f4xx_it.c.
 */
void USART2_IRQHandler(void){HAL_UART_IRQHandler(&huart2);}
void Demo_Init(void) {
    HAL_NVIC_SetPriority(USART2_IRQn,0,0);
    HAL_NVIC_EnableIRQ(USART2_IRQn);
    if(HAL_UART_Receive_IT(&huart2,&rx_byte,1U)!=HAL_OK)rx_restart=1;
    help();serial("Inicializando pantalla...\r\n");lcd_init();screen(0,1);report();serial("\r\nComando> ");
}
void Demo_Poll(void) {
    if(rx_restart){
        HAL_NVIC_DisableIRQ(USART2_IRQn);HAL_UART_AbortReceive(&huart2);
        __HAL_UART_CLEAR_OREFLAG(&huart2);rx_restart=0;
        if(HAL_UART_Receive_IT(&huart2,&rx_byte,1U)!=HAL_OK)rx_restart=1;
        HAL_NVIC_EnableIRQ(USART2_IRQn);
    }
    if(rx_overflow){
        HAL_NVIC_DisableIRQ(USART2_IRQn);rx_tail=rx_head;rx_overflow=0;HAL_NVIC_EnableIRQ(USART2_IRQn);
        line_len=0;line_bad=1;serial("\r\nError/desborde serial: presiona ENTER y reenvia el comando.\r\n");
    }
    while(rx_tail!=rx_head){uint8_t c=rx_ring[rx_tail];rx_tail=(uint16_t)((rx_tail+1U)%RX_SIZE);
        if(c=='\r'||c=='\n'){
            if(line_bad){serial("\r\nLinea descartada.\r\nComando> ");}
            else if(line_len){line[line_len]=0;serial("\r\n");process(line);serial("\r\nComando> ");}
            line_len=0;line_bad=0;
        }else if(c==8 || c==127){if(line_len)line_len--;}
        else if(c>=32 && c<=126){if(line_len<sizeof(line)-1U)line[line_len++]=(char)c;else line_bad=1;}
        else if(c=='\t'){if(line_len<sizeof(line)-1U)line[line_len++]=' ';else line_bad=1;}
    }
}
