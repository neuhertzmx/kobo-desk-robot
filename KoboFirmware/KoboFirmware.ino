/*
  ============================================================
   KOBO — Firmware v1.0
   Placa: Waveshare ESP32-S3-Touch-LCD-1.69 (ST7789V2 240x280)
   Alimentación: solo USB (sin batería)
  ============================================================

   QUÉ HACE
   - Ojos animados: parpadea y mira a su alrededor solo.
   - Toque corto      -> feliz + chirrido
   - Mantener 1 s     -> ojos de corazón + melodía
   - 5 toques rápidos -> se enoja
   - 20 s sin tocarlo -> le da sueño (bosteza)
   - 45 s sin tocarlo -> se duerme (Zzz, baja el brillo)
   - Tocarlo dormido  -> se despierta sorprendido

   LIBRERÍA NECESARIA (Arduino IDE > Gestor de librerías)
   - "GFX Library for Arduino" de moononournation

   AJUSTES EN Herramientas (Tools)
   - Placa: "ESP32S3 Dev Module"
   - USB CDC On Boot: Enabled
   - PSRAM: "OPI PSRAM"
   - Flash Size: 16MB
  ============================================================
*/

#include <Arduino_GFX_Library.h>
#include <Wire.h>

// Estados de ánimo (va aquí arriba para que el IDE de Arduino lo vea
// antes de generar los prototipos de las funciones)
enum Animo { NORMAL, FELIZ, AMOR, SORPRESA, ENOJO, SUENO, DORMIDO };

// ---------------- PINES (placa nueva) ----------------
#define LCD_DC    4
#define LCD_CS    5
#define LCD_SCK   6
#define LCD_MOSI  7
#define LCD_RST   8
#define LCD_BL    15

#define TP_SDA    11
#define TP_SCL    10
#define TP_RST    13
#define TP_ADDR   0x15   // controlador táctil CST816T

#define BUZZER    42
#define SYS_EN    41     // mantiene encendida la placa

// ---------------- AJUSTES DE PERSONALIDAD ----------------
const uint32_t TIEMPO_SUENO     = 20000;  // ms sin tocar para que le dé sueño
const uint32_t TIEMPO_DORMIR    = 45000;  // ms sin tocar para dormirse
const uint32_t TIEMPO_PRESION   = 1000;   // ms para "mantener presionado"
const bool     RONQUIDOS        = true;   // false = duerme en silencio
const uint8_t  BRILLO_NORMAL    = 255;
const uint8_t  BRILLO_DORMIDO   = 40;

// Colores (RGB565)
#define C_FONDO   0x0000   // negro
#define C_OJO     0x07FF   // cian
#define C_AMOR    0xF80F   // rosa
#define C_ENOJO   0xF800   // rojo
#define C_TEXTO   0x7BEF   // gris

// ---------------- PANTALLA ----------------
Arduino_DataBus *bus = new Arduino_ESP32SPI(LCD_DC, LCD_CS, LCD_SCK, LCD_MOSI);
Arduino_GFX *panel = new Arduino_ST7789(bus, LCD_RST, 0 /*rotación*/, true /*IPS*/,
                                        240, 280, 0, 20, 0, 20);
Arduino_Canvas *gfx = new Arduino_Canvas(240, 280, panel);  // dibuja sin parpadeo

const int W = 240, H = 280;

// ============================================================
//   SONIDOS  — pega aquí tus favoritos de KoboSonidos.ino
//   Formato: {frecuencia_Hz, duración_ms}; 0 Hz = silencio
//   Cada sonido termina con {0, 0}
// ============================================================
struct Nota { uint16_t f; uint16_t ms; };

const Nota SND_ARRANQUE[] = { {523,90},{659,90},{784,90},{1047,180},{0,0} };
const Nota SND_FELIZ[]    = { {1319,60},{0,20},{1568,60},{0,20},{2093,110},{0,0} };
const Nota SND_AMOR[]     = { {784,140},{988,140},{1175,140},{1568,300},{0,0} };
const Nota SND_SORPRESA[] = { {600,40},{900,40},{1300,40},{1800,40},{2400,90},{0,0} };
const Nota SND_ENOJO[]    = { {220,120},{0,40},{196,120},{0,40},{165,250},{0,0} };
const Nota SND_BOSTEZO[]  = { {900,80},{800,80},{700,80},{600,100},{500,140},{420,200},{0,0} };
const Nota SND_RONQUIDO[] = { {110,250},{0,120},{90,350},{0,0} };

// ---------------- REPRODUCTOR SIN BLOQUEO ----------------
// Suena mientras los ojos siguen animándose.
const Nota *sndActual = nullptr;
uint8_t     sndIdx    = 0;
uint32_t    sndFin    = 0;

void tocar(const Nota *s) {
  sndActual = s;
  sndIdx = 0;
  sndFin = 0;
}

void actualizarSonido() {
  if (!sndActual || millis() < sndFin) return;
  const Nota &n = sndActual[sndIdx];
  if (n.ms == 0) {               // fin del sonido
    noTone(BUZZER);
    sndActual = nullptr;
    return;
  }
  if (n.f > 0) tone(BUZZER, n.f);
  else         noTone(BUZZER);
  sndFin = millis() + n.ms;
  sndIdx++;
}

// ---------------- TÁCTIL (CST816T, sin librería) ----------------
void iniciarTactil() {
  pinMode(TP_RST, OUTPUT);
  digitalWrite(TP_RST, LOW);  delay(10);
  digitalWrite(TP_RST, HIGH); delay(50);
  Wire.begin(TP_SDA, TP_SCL);
  // Desactiva el auto-sleep del táctil para que siempre responda
  Wire.beginTransmission(TP_ADDR);
  Wire.write(0xFE);
  Wire.write(0x01);
  Wire.endTransmission();
}

bool hayToque() {
  Wire.beginTransmission(TP_ADDR);
  Wire.write(0x02);                         // registro: número de dedos
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom((uint8_t)TP_ADDR, (uint8_t)1) != 1) return false;
  return (Wire.read() & 0x0F) > 0;
}

// ---------------- ESTADO DE KOBO ----------------
Animo    animo       = NORMAL;
uint32_t animoFin    = 0;     // cuándo regresa a NORMAL (0 = no expira)
uint32_t ultimoToque = 0;

// Mirada
float miraX = 0, miraY = 0, objX = 0, objY = 0;
uint32_t proximaMirada = 0;

// Parpadeo
uint32_t proximoParpadeo = 0, inicioParpadeo = 0;
bool     parpadeando = false;
const uint16_t DUR_PARPADEO = 160;

// Apertura de ojos al arrancar
float apertura = 0.0;

// Toques
bool     tocando = false, presionLarga = false;
uint32_t inicioToque = 0;
uint32_t toques[5] = {0};
uint8_t  toqueIdx = 0;

// Ronquidos y "Zzz"
uint32_t proximoRonquido = 0;

void cambiarAnimo(Animo a, uint32_t duracion) {
  animo = a;
  animoFin = duracion ? millis() + duracion : 0;
  analogWrite(LCD_BL, a == DORMIDO ? BRILLO_DORMIDO : BRILLO_NORMAL);
}

// ---------------- DIBUJO ----------------
void dibujarCorazon(int cx, int cy, int s, uint16_t c) {
  int r = s / 4;
  gfx->fillCircle(cx - r, cy - r / 2, r, c);
  gfx->fillCircle(cx + r, cy - r / 2, r, c);
  gfx->fillTriangle(cx - 2 * r, cy - r / 3, cx + 2 * r, cy - r / 3, cx, cy + s / 2, c);
}

void dibujarOjo(int cx, int cy, bool izquierdo, float abierto) {
  int w = 62, h = 78;
  if (animo == SORPRESA) { w = 72; h = 90; }

  int x = cx - w / 2 + (int)miraX;
  int yc = cy + (int)miraY;

  switch (animo) {
    case AMOR:
      dibujarCorazon(cx + (int)miraX, yc, 70, C_AMOR);
      return;

    case DORMIDO:
      gfx->fillRoundRect(x, yc, w, 6, 3, C_OJO);
      return;

    case FELIZ: {
      // Ojo "^": rectángulo redondeado tapado por abajo
      gfx->fillRoundRect(x, yc - h / 2, w, h, 28, C_OJO);
      gfx->fillRoundRect(x - 4, yc - h / 2 + 34, w + 8, h, 28, C_FONDO);
      return;
    }

    default: {
      float a = abierto;
      if (animo == SUENO) a *= 0.35;
      int eh = max(6, (int)(h * a));
      uint16_t c = (animo == ENOJO) ? C_ENOJO : C_OJO;
      gfx->fillRoundRect(x, yc - eh / 2, w, eh, min(20, eh / 2), c);

      if (animo == ENOJO) {
        // Ceja inclinada hacia el centro de la cara
        int top = yc - eh / 2 - 1;
        int caida = eh / 2;
        if (izquierdo)
          gfx->fillTriangle(x - 2, top, x + w + 2, top, x + w + 2, top + caida, C_FONDO);
        else
          gfx->fillTriangle(x - 2, top, x + w + 2, top, x - 2, top + caida, C_FONDO);
      }
    }
  }
}

void dibujarBoca(int cy) {
  int cx = W / 2 + (int)miraX;
  int y = cy + (int)miraY;
  switch (animo) {
    case FELIZ:
    case AMOR:
      gfx->fillCircle(cx, y, 16, animo == AMOR ? C_AMOR : C_OJO);
      gfx->fillRect(cx - 18, y - 18, 36, 18, C_FONDO);   // deja media luna
      gfx->fillCircle(cx, y - 4, 10, C_FONDO);
      break;
    case SORPRESA:
      gfx->fillCircle(cx, y + 4, 10, C_OJO);
      gfx->fillCircle(cx, y + 4, 5, C_FONDO);
      break;
    default:
      break;  // sin boca: cara limpia
  }
}

void dibujarZzz() {
  // Las "Z" suben y bajan lentamente
  int dy = (millis() / 60) % 30;
  gfx->setTextColor(C_TEXTO);
  gfx->setTextSize(2); gfx->setCursor(170, 80 - dy / 2);  gfx->print("z");
  gfx->setTextSize(3); gfx->setCursor(190, 55 - dy / 2);  gfx->print("Z");
}

void dibujarCara() {
  gfx->fillScreen(C_FONDO);

  // Factor de parpadeo (1 = abierto, 0 = cerrado)
  float factor = 1.0;
  if (parpadeando) {
    float t = (millis() - inicioParpadeo) / (float)DUR_PARPADEO;
    factor = (t < 0.5) ? 1.0 - t * 2.0 : (t - 0.5) * 2.0;
    factor = constrain(factor, 0.05, 1.0);
  }
  float abierto = factor * apertura;

  const int ojoY = 125;
  dibujarOjo(W / 2 - 52, ojoY, true, abierto);
  dibujarOjo(W / 2 + 52, ojoY, false, abierto);
  dibujarBoca(205);
  if (animo == DORMIDO) dibujarZzz();

  gfx->flush();
}

// ---------------- COMPORTAMIENTO ----------------
void alTocar() {
  uint32_t ahora = millis();
  ultimoToque = ahora;

  if (animo == DORMIDO || animo == SUENO) {
    cambiarAnimo(SORPRESA, 1200);
    tocar(SND_SORPRESA);
    return;
  }

  // ¿5 toques en menos de 3 s? -> enojo
  toques[toqueIdx] = ahora;
  toqueIdx = (toqueIdx + 1) % 5;
  uint32_t masViejo = toques[toqueIdx];
  if (masViejo != 0 && ahora - masViejo < 3000) {
    cambiarAnimo(ENOJO, 2500);
    tocar(SND_ENOJO);
    memset(toques, 0, sizeof(toques));
    return;
  }

  if (animo != ENOJO) {
    cambiarAnimo(FELIZ, 1500);
    tocar(SND_FELIZ);
  }
}

void alPresionarLargo() {
  ultimoToque = millis();
  cambiarAnimo(AMOR, 2500);
  tocar(SND_AMOR);
}

void leerTactil() {
  bool t = hayToque();
  uint32_t ahora = millis();

  if (t && !tocando) {                 // empezó el toque
    tocando = true;
    presionLarga = false;
    inicioToque = ahora;
  } else if (t && tocando) {           // sigue tocando
    if (!presionLarga && ahora - inicioToque > TIEMPO_PRESION) {
      presionLarga = true;
      alPresionarLargo();
    }
  } else if (!t && tocando) {          // soltó
    tocando = false;
    if (!presionLarga && ahora - inicioToque > 30) alTocar();
  }
}

void actualizarComportamiento() {
  uint32_t ahora = millis();

  // Regresar a NORMAL cuando termina una emoción
  if (animoFin && ahora > animoFin) cambiarAnimo(NORMAL, 0);

  // Sueño por inactividad
  uint32_t inactivo = ahora - ultimoToque;
  if (animo == NORMAL && inactivo > TIEMPO_SUENO) {
    cambiarAnimo(SUENO, 0);
    tocar(SND_BOSTEZO);
  }
  if (animo == SUENO && inactivo > TIEMPO_DORMIR) {
    cambiarAnimo(DORMIDO, 0);
    proximoRonquido = ahora + 3000;
  }
  if (animo == DORMIDO && RONQUIDOS && ahora > proximoRonquido) {
    tocar(SND_RONQUIDO);
    proximoRonquido = ahora + random(4000, 7000);
  }

  // Mirar alrededor (solo despierto)
  if (animo == NORMAL || animo == SUENO) {
    if (ahora > proximaMirada) {
      if (random(3) == 0) { objX = 0; objY = 0; }          // a veces mira al frente
      else { objX = random(-22, 23); objY = random(-14, 15); }
      proximaMirada = ahora + random(1200, 4000);
    }
  } else {
    objX = 0; objY = 0;
  }
  miraX += (objX - miraX) * 0.18;   // movimiento suave
  miraY += (objY - miraY) * 0.18;

  // Parpadeo aleatorio
  if (animo != DORMIDO && animo != FELIZ && animo != AMOR) {
    if (!parpadeando && ahora > proximoParpadeo) {
      parpadeando = true;
      inicioParpadeo = ahora;
    }
  }
  if (parpadeando && ahora - inicioParpadeo > DUR_PARPADEO) {
    parpadeando = false;
    // a veces parpadea doble
    proximoParpadeo = ahora + (random(5) == 0 ? 180 : random(2000, 6000));
  }

  // Abrir los ojos poco a poco al arrancar
  if (apertura < 1.0) apertura = min(1.0f, apertura + 0.03f);
}

// ---------------- SETUP / LOOP ----------------
void setup() {
  pinMode(SYS_EN, OUTPUT);
  digitalWrite(SYS_EN, HIGH);

  Serial.begin(115200);
  randomSeed(esp_random());

  pinMode(BUZZER, OUTPUT);
  pinMode(LCD_BL, OUTPUT);
  analogWrite(LCD_BL, BRILLO_NORMAL);

  if (!gfx->begin()) Serial.println("Error al iniciar la pantalla");
  gfx->fillScreen(C_FONDO);
  gfx->flush();

  iniciarTactil();

  ultimoToque = millis();
  proximoParpadeo = millis() + 2500;
  tocar(SND_ARRANQUE);
  Serial.println("Kobo listo");
}

void loop() {
  static uint32_t ultimoCuadro = 0;
  actualizarSonido();

  if (millis() - ultimoCuadro >= 33) {   // ~30 cuadros por segundo
    ultimoCuadro = millis();
    leerTactil();
    actualizarComportamiento();
    dibujarCara();
  }
}
