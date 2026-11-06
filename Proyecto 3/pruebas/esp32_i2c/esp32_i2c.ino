#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

const char* ssid = "ChiFlipZ6";
const char* password = "DinoTotoro";
constexpr int PIN_SDA=21, PIN_SCL=22;
constexpr uint8_t DIRECCION_STM1=0x10, DIRECCION_STM2=0x11;
constexpr uint32_t FRECUENCIA_I2C=100000;
constexpr uint32_t PERIODO_CONSULTA_MS=250;
constexpr uint32_t VIGENCIA_MS=2000;
WebServer server(80);

struct Estado {
  uint8_t global=0, local1=0, local2=0;
  bool lectura1=false, lectura2=false, recibido=false, bus=false;
  bool envioIntentado=false;
  uint8_t envio1=255, envio2=255;
  uint32_t fecha=0;
};
Estado compartido;
portMUX_TYPE candado=portMUX_INITIALIZER_UNLOCKED;
// Prototipos explicitos para el generador de sketches Arduino.
Estado copiarEstado();
void publicarEstado(const Estado &dato);

// Solo se copia memoria bajo el candado: nunca I2C, serial o HTTP.
Estado copiarEstado() {
  portENTER_CRITICAL(&candado);
  Estado copia=compartido;
  portEXIT_CRITICAL(&candado);
  return copia;
}
void publicarEstado(const Estado &dato) {
  portENTER_CRITICAL(&candado);
  compartido=dato;
  portEXIT_CRITICAL(&candado);
}
uint8_t contarOcupados(uint8_t dato) {
  uint8_t total=0;
  for(uint8_t i=0;i<8;i++)total+=(dato>>i)&1U;
  return total;
}
void obtenerBits(uint8_t dato,char *salida,uint8_t cantidad) {
  for(uint8_t i=0;i<cantidad;i++)salida[i]=((dato>>(cantidad-1-i))&1U)?'1':'0';
  salida[cantidad]=0;
}
bool leerEstadoLocal(uint8_t direccion,uint8_t &estado) {
  size_t cantidad=Wire.requestFrom(direccion,static_cast<size_t>(1),true);
  if(cantidad!=1U || Wire.available()<1) {
    while(Wire.available()>0)Wire.read();
    return false;
  }
  int recibido=Wire.read();
  if(recibido<0)return false;
  uint8_t dato=static_cast<uint8_t>(recibido);
  if((dato&0xF0U)!=0U) {
    Serial.printf("Dato local invalido de 0x%02X: 0x%02X\r\n",(unsigned)direccion,(unsigned)dato);
    return false;
  }
  estado=dato;
  return true;
}
uint8_t enviarEstadoGlobal(uint8_t direccion,uint8_t estado) {
  Wire.beginTransmission(direccion);
  Wire.write(estado);
  return Wire.endTransmission(true);
}
// Esta funcion mantiene el protocolo de un byte sin comandos adicionales.
// Solo esta tarea usa Wire. Wi-Fi y WebServer se atienden desde loop().
void tareaI2C(void *) {
  Estado dato;
  uint32_t ultimoIntento=millis()-2000U;
  for(;;) {
    uint32_t inicio=millis();
    if(!dato.bus && (uint32_t)(inicio-ultimoIntento)>=2000U) {
      ultimoIntento=inicio;
      dato.bus=Wire.begin(PIN_SDA,PIN_SCL,FRECUENCIA_I2C);
      if(dato.bus)Wire.setTimeOut(50);
      else Serial.println("ERROR: no se pudo iniciar I2C; se reintentara.");
    }
    dato.lectura1=false;dato.lectura2=false;dato.envioIntentado=false;
    dato.envio1=255;dato.envio2=255;
    if(dato.bus) {
      // Evaluar ambas, incluso cuando falla la primera.
      dato.lectura1=leerEstadoLocal(DIRECCION_STM1,dato.local1);
      dato.lectura2=leerEstadoLocal(DIRECCION_STM2,dato.local2);
      if(dato.lectura1 && dato.lectura2) {
        dato.global=static_cast<uint8_t>((dato.local1<<4)|dato.local2);
        dato.fecha=millis();dato.recibido=true;
        dato.envioIntentado=true;
        // Reenviar SIEMPRE, aunque no cambien bits, para refrescar los STM.
        dato.envio1=enviarEstadoGlobal(DIRECCION_STM1,dato.global);
        dato.envio2=enviarEstadoGlobal(DIRECCION_STM2,dato.global);
      }
    }
    publicarEstado(dato);
    if(dato.lectura1 && dato.lectura2) {
      char a[5],b[5],global[9];
      obtenerBits(dato.local1,a,4);obtenerBits(dato.local2,b,4);obtenerBits(dato.global,global,8);
      unsigned ocupados=contarOcupados(dato.global);
      Serial.printf("STM1 [P1-P4]: %s | STM2 [P5-P8]: %s\r\n",a,b);
      Serial.printf("GLOBAL [P1-P8]: %s | HEX: 0x%02X | Ocupados: %u | Libres: %u\r\n",
                    global,(unsigned)dato.global,ocupados,8U-ocupados);
      Serial.printf("Envio global -> STM1: %s (codigo %u) | STM2: %s (codigo %u)\r\n",
                    dato.envio1==0?"OK":"ERROR",(unsigned)dato.envio1,
                    dato.envio2==0?"OK":"ERROR",(unsigned)dato.envio2);
    } else {
      Serial.printf("Sin estado global actualizado | STM1: %s | STM2: %s\r\n",
                    dato.lectura1?"lectura OK":"sin dato valido",dato.lectura2?"lectura OK":"sin dato valido");
      Serial.println("No se envia estado global. Las placas detectaran datos vencidos.");
    }
    Serial.println("----------------------------------------");
    uint32_t duracion=millis()-inicio;
    // Si un ciclo se prolonga, ceder CPU sin encadenar consultas atrasadas.
    vTaskDelay(pdMS_TO_TICKS(duracion<PERIODO_CONSULTA_MS?PERIODO_CONSULTA_MS-duracion:10U));
  }
}

const char PAGINA[] PROGMEM=R"HTML(
<!doctype html><html lang="es"><head><meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Parqueo-matic</title><style>
*{box-sizing:border-box}body{font-family:Arial,sans-serif;background:#eef2f6;color:#243247;margin:0;padding:24px}
main{max-width:780px;margin:auto}h1{margin-bottom:8px}.nota{color:#586779}
.resumen{display:flex;gap:16px;margin:24px 0}.contador{flex:1;background:white;border-radius:12px;padding:18px;text-align:center}
.contador strong{display:block;font-size:38px;margin-top:8px}.grupo{display:grid;grid-template-columns:repeat(4,1fr);gap:12px;margin-bottom:24px}
.espacio{padding:24px 6px;border-radius:10px;text-align:center;background:#dce2e8}.espacio strong{display:block;font-size:26px;margin-bottom:10px}
.libre{background:#187a46;color:white}.ocupado{background:#b82d36;color:white}
#conexion{font-weight:bold}#retorno{color:#725100}code{background:white;padding:6px 10px;border-radius:6px}
@media(max-width:460px){body{padding:14px}.grupo{gap:6px}.espacio{font-size:12px}}
</style></head><body><main>
<h1>Parqueo-matic</h1><p class="nota">Monitoreo del nivel · 8 espacios · Sensores por I2C</p>
<p id="conexion" role="status">Conectando con el ESP32...</p>
<div class="resumen"><div class="contador">Disponibles<strong id="libres">—</strong></div>
<div class="contador">Ocupados<strong id="ocupados">—</strong></div></div>
<h2>STM1 · P1–P4</h2><div class="grupo" id="grupoA"></div>
<h2>STM2 · P5–P8</h2><div class="grupo" id="grupoB"></div>
<p>Estado: <code id="bits">--------</code></p><p id="retorno"></p>
<p class="nota">Verde = libre · Rojo = ocupado · Gris = sin datos válidos</p>
</main><script>
const el=id=>document.getElementById(id);
for(let p=1;p<=8;p++){
 const tarjeta=document.createElement('div');tarjeta.id='p'+p;tarjeta.className='espacio';
 tarjeta.innerHTML='<strong>P'+p+'</strong><span>Sin datos</span>';
 el(p<=4?'grupoA':'grupoB').appendChild(tarjeta);
}
function desconocido(mensaje){
 el('conexion').textContent=mensaje;el('libres').textContent='—';el('ocupados').textContent='—';el('bits').textContent='--------';
 for(let p=1;p<=8;p++){el('p'+p).className='espacio';el('p'+p).querySelector('span').textContent='Sin datos';}
 el('retorno').textContent='';
}
async function actualizar(){
 const control=new AbortController();const temporizador=setTimeout(()=>control.abort(),1500);
 try{
  const r=await fetch('/estado',{cache:'no-store',signal:control.signal});if(!r.ok)throw Error('HTTP');
  const d=await r.json();
  if(!d.valido){
   desconocido(d.bus?'ESP32 conectado · Esperando datos válidos de ambos STM':'ESP32 conectado · I2C no disponible');
   el('retorno').textContent='Lectura STM1: '+(d.stm1?'OK':'sin dato')+' · STM2: '+(d.stm2?'OK':'sin dato');
  }else{
   if(typeof d.bits!=='string'||!/^[01]{8}$/.test(d.bits))throw Error('Formato');
   for(let i=0;i<8;i++){
    const ocupado=d.bits[i]==='1',t=el('p'+(i+1));t.className='espacio '+(ocupado?'ocupado':'libre');
    t.querySelector('span').textContent=ocupado?'Ocupado':'Libre';
   }
   el('libres').textContent=d.libres;el('ocupados').textContent=d.ocupados;el('bits').textContent=d.bits;
   el('conexion').textContent='Conectado · Datos de sensores actualizados';
   el('retorno').textContent=d.sincronizado?'Envío global aceptado por ambos STM':
    'Aviso: fallo en retorno I2C. STM1: '+d.envio1+' · STM2: '+d.envio2+' (0 = OK). Los indicadores físicos podrían estar desactualizados.';
  }
 }catch(e){desconocido('Sin conexión con el ESP32');}
 finally{clearTimeout(temporizador);setTimeout(actualizar,250);}
}
actualizar();
</script></body></html>
)HTML";

void mostrarPagina() {
  server.sendHeader("Cache-Control","no-store");
  server.send_P(200,"text/html; charset=utf-8",PAGINA);
}
void enviarEstado() {
  Estado e=copiarEstado();
  uint32_t edad=millis()-e.fecha;
  bool valido=e.bus && e.recibido && e.lectura1 && e.lectura2 && edad<=VIGENCIA_MS;
  bool sincronizado=valido && e.envioIntentado && e.envio1==0 && e.envio2==0;
  char bits[9]="--------";
  if(valido)obtenerBits(e.global,bits,8);
  char libres[8]="null",ocupados[8]="null";
  if(valido){unsigned n=contarOcupados(e.global);snprintf(libres,sizeof libres,"%u",8U-n);snprintf(ocupados,sizeof ocupados,"%u",n);}
  char respuesta[384];
  snprintf(respuesta,sizeof respuesta,
    "{\"valido\":%s,\"bits\":\"%s\",\"libres\":%s,\"ocupados\":%s,"
    "\"bus\":%s,\"stm1\":%s,\"stm2\":%s,\"sincronizado\":%s,\"envio1\":%u,\"envio2\":%u,\"edad_ms\":%lu}",
    valido?"true":"false",bits,libres,ocupados,e.bus?"true":"false",
    e.lectura1?"true":"false",e.lectura2?"true":"false",sincronizado?"true":"false",
    (unsigned)e.envio1,(unsigned)e.envio2,(unsigned long)edad);
  server.sendHeader("Cache-Control","no-store");
  server.send(200,"application/json; charset=utf-8",respuesta);
}
void setup() {
  Serial.begin(115200);
  Serial.println("\r\nESP32: I2C + servidor web del parqueo");
  Serial.println("SDA21 SCL22 | 100kHz | STM1=0x10, STM2=0x11 | 1=ocupado, 0=libre");
  // Iniciar I2C incluso si no existe Wi-Fi.
  if(xTaskCreate(tareaI2C,"ParqueoI2C",4096,nullptr,2,nullptr)!=pdPASS)
    Serial.println("ERROR: no se pudo crear tarea I2C. Reinicia el ESP32.");
  server.on("/",HTTP_GET,mostrarPagina);
  server.on("/estado",HTTP_GET,enviarEstado);
  server.onNotFound([](){server.send(404,"text/plain","Pagina no encontrada");});
  WiFi.mode(WIFI_STA);WiFi.setAutoReconnect(true);WiFi.begin(ssid,password);
  Serial.println("Conectando a Wi-Fi; I2C sigue funcionando independientemente.");
}
void loop() {
  static bool servidorIniciado=false,wifiAnterior=false;
  static uint32_t ultimoMensaje=0,ultimoReintento=0;
  bool conectado=WiFi.status()==WL_CONNECTED;
  if(conectado) {
    if(!servidorIniciado){server.begin();servidorIniciado=true;}
    if(!wifiAnterior){Serial.print("Pagina disponible en: http://");Serial.println(WiFi.localIP());}
    server.handleClient();
  }else{
    uint32_t ahora=millis();
    if((uint32_t)(ahora-ultimoMensaje)>=5000U){ultimoMensaje=ahora;Serial.println("Esperando Wi-Fi; consultas I2C continúan.");}
    if((uint32_t)(ahora-ultimoReintento)>=15000U){ultimoReintento=ahora;WiFi.reconnect();}
  }
  wifiAnterior=conectado;
  delay(1);
}
