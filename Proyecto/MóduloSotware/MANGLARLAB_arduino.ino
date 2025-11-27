// ======================================================
//               M A N G L A R L A B  – ESP32
//   Agua: DS18B20  (temperatura)
//   Aire: DHT11    (temperatura y humedad)
//   pH:   módulo analógico (ADC34)
//   EC:   TDS v1.0 (ADC35)  -> EC25 -> Salinidad (ppt)
//   Nivel: HC-SR04 (TRIG 5, ECHO 18)
//   LCD I2C 16x2 (0x27, SDA 21, SCL 22)
//   LEDs, Ventilador y Buzzer
//   + WiFi + JSON /datos para MIT App Inventor
//   + Panel Web con HTML embebido
// ======================================================

#include <WiFi.h>
#include <WebServer.h>

#include <OneWire.h>
#include <DallasTemperature.h>
#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <math.h>

// ===================== WIFI =====================

// Hotspot del celular
const char* ssid     = "88888888";
const char* password = "88888888";

// Sin IP fija: el hotspot asigna una IP dinámica (10.90.64.x)
WebServer server(80);

// ===================== PINES =====================

// DS18B20 (temperatura agua)
#define ONE_WIRE_BUS 4

// Ultrasonido
#define TRIG_PIN 5
#define ECHO_PIN 18

// pH (analógico)
#define PH_PIN 34

// TDS / Conductividad (analógico)
#define TDS_PIN 35

// DHT (aire)
#define DHTPIN 14
#define DHTTYPE DHT11

// LEDs, ventilador y buzzer
#define LED_VERDE 26
#define LED_ROJO  27
#define FAN_PIN   25
#define BUZZER_PIN 12

// ===================== OBJETOS =====================

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature ds18b20(&oneWire);
DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 16, 2);

// ===================== VARIABLES GLOBALES =====================

float ultimaDistancia = -1;
float tempAguaC = 25.0;     // guarda el último valor válido del DS18B20

// manejo de pantalla (datos vs logo)
bool showDataScreen = true;
unsigned long lastScreenChange = 0;

// Para compartir últimos valores (para JSON)
float ultimo_tAire = NAN;
float ultimo_hAire = NAN;
float ultimo_nivel = NAN;
float ultimo_ph    = NAN;
float ultimo_sal   = NAN;

// ===================== HTML EMBEBIDO =====================
const char MAIN_page[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="es">
<head>
  <meta charset="UTF-8">
  <title>ManglarLab – Panel</title>
  <meta name="viewport" content="width=device-width,initial-scale=1.0">
  <style>
    :root{
      --bg:#101518;
      --card:#1b242b;
      --accent:#00c896;
      --accent-soft:#00c89622;
      --text:#f5f5f5;
      --muted:#9ca3af;
      --danger:#f97373;
    }
    *{box-sizing:border-box;margin:0;padding:0;font-family:system-ui,-apple-system,BlinkMacSystemFont,"Segoe UI",sans-serif;}
    body{
      background:radial-gradient(circle at top,#1f2933 0,#050608 55%);
      color:var(--text);
      padding:20px;
      display:flex;
      justify-content:center;
    }
    .container{
      max-width:960px;
      width:100%;
    }
    .header{
      display:flex;
      justify-content:space-between;
      align-items:center;
      margin-bottom:18px;
      gap:10px;
    }
    .title{
      font-size:1.5rem;
      font-weight:700;
    }
    .subtitle{
      font-size:0.85rem;
      color:var(--muted);
    }
    .badge{
      padding:3px 8px;
      border-radius:999px;
      font-size:0.7rem;
      background:var(--accent-soft);
      color:var(--accent);
      border:1px solid var(--accent);
    }
    .card{
      background:linear-gradient(135deg,#141b21,#111827);
      border-radius:16px;
      padding:18px;
      box-shadow:0 15px 40px rgba(0,0,0,.45);
      border:1px solid #1f2933;
    }
    .grid{
      display:grid;
      grid-template-columns:repeat(auto-fit,minmax(170px,1fr));
      gap:14px;
      margin-top:14px;
    }
    .metric{
      background:#02061755;
      border-radius:14px;
      padding:12px 12px 10px;
      border:1px solid #1f2937;
      display:flex;
      flex-direction:column;
      gap:4px;
    }
    .metric-title{
      font-size:0.8rem;
      color:var(--muted);
      display:flex;
      justify-content:space-between;
      align-items:center;
    }
    .metric-value{
      font-size:1.4rem;
      font-weight:600;
      margin-top:2px;
    }
    .metric-unit{
      font-size:0.75rem;
      color:var(--muted);
      margin-left:4px;
    }
    .metric-status{
      font-size:0.72rem;
      padding:2px 7px;
      border-radius:999px;
      border:1px solid #374151;
      color:var(--muted);
    }
    .metric-danger{
      color:var(--danger);
      border-color:var(--danger);
      background:#7f1d1d55;
    }
    .footer-row{
      margin-top:14px;
      display:flex;
      flex-wrap:wrap;
      justify-content:space-between;
      align-items:center;
      gap:10px;
      font-size:0.8rem;
      color:var(--muted);
    }
    .controls{
      display:flex;
      align-items:center;
      gap:8px;
      flex-wrap:wrap;
    }
    button{
      border:none;
      border-radius:999px;
      padding:7px 16px;
      font-size:0.8rem;
      cursor:pointer;
      background:var(--accent);
      color:#02110c;
      font-weight:600;
      box-shadow:0 8px 20px rgba(0,200,150,.35);
      transition:transform .12s ease, box-shadow .12s ease, background .12s ease;
    }
    button:hover{
      transform:translateY(-1px);
      box-shadow:0 10px 25px rgba(0,200,150,.45);
      background:#0fe3b0;
    }
    button:active{
      transform:translateY(1px);
      box-shadow:0 3px 10px rgba(0,0,0,.6);
    }
    label.chk{
      display:inline-flex;
      align-items:center;
      gap:5px;
      cursor:pointer;
    }
    .chart-card{
      margin-top:16px;
      background:#02061788;
      border-radius:16px;
      padding:14px 16px 10px;
      border:1px solid #1f2937;
    }
    .chart-title{
      font-size:0.9rem;
      color:var(--muted);
      margin-bottom:6px;
      display:flex;
      justify-content:space-between;
      align-items:center;
    }
    .chart-sub{
      font-size:0.75rem;
      color:var(--muted);
      margin-top:4px;
    }
    canvas{
      width:100%;
      max-width:920px;
      border-radius:10px;
      background:#020617;
    }
    @media (max-width:600px){
      .header{flex-direction:column;align-items:flex-start;}
    }
  </style>
</head>
<body>
  <div class="container">
    <div class="header">
      <div>
        <div class="title">ManglarLab · ESP32</div>
        <div class="subtitle">Monitoreo de manglar: pH, salinidad y nivel del agua en tiempo casi real.</div>
      </div>
      <div>
        <div class="badge">Prototipo</div>
      </div>
    </div>

    <div class="card">
      <div class="grid">
        <div class="metric">
          <div class="metric-title">
            <span>Temperatura aire</span>
            <span class="metric-status" id="estado_aire">–</span>
          </div>
          <div class="metric-value">
            <span id="temp_aire">--</span>
            <span class="metric-unit">°C</span>
          </div>
        </div>

        <div class="metric">
          <div class="metric-title">
            <span>pH del agua</span>
            <span class="metric-status" id="estado_ph">–</span>
          </div>
          <div class="metric-value">
            <span id="ph">--</span>
          </div>
        </div>

        <div class="metric">
          <div class="metric-title">
            <span>Salinidad</span>
            <span class="metric-status" id="estado_sal">–</span>
          </div>
          <div class="metric-value">
            <span id="salinidad">--</span>
            <span class="metric-unit">ppt</span>
          </div>
        </div>

        <div class="metric">
          <div class="metric-title">
            <span>Nivel de agua</span>
            <span class="metric-status" id="estado_nivel">–</span>
          </div>
          <div class="metric-value">
            <span id="nivel">--</span>
            <span class="metric-unit">cm</span>
          </div>
        </div>
      </div>

      <div class="chart-card">
        <div class="chart-title">
          <span>Evolución del nivel de agua</span>
          <span style="font-size:0.75rem;color:#9ca3af;">Últimos 30 puntos</span>
        </div>
        <canvas id="nivelChart" width="880" height="220"></canvas>
        <div class="chart-sub">
          Eje X: tiempo (hh:mm:ss). · Eje Y: nivel del agua (cm). Si el nivel sube demasiado, el patrón se ve aquí.
        </div>
      </div>

      <div class="footer-row">
        <div>
          Última actualización:
          <span id="last_update">--:--:--</span>
        </div>
        <div class="controls">
          <button onclick="actualizar()">Actualizar ahora</button>
          <label class="chk">
            <input type="checkbox" id="auto_chk" onchange="toggleAuto()"> Auto cada 10 s
          </label>
        </div>
      </div>
    </div>
  </div>

<script>
let autoTimer = null;
let nivelData = [];
let timeLabels = [];
const MAX_POINTS = 30;

function formatearHora(d){
  return d.toLocaleTimeString('es-PE',{hour12:false});
}

function actualizar(){
  fetch('/json')
    .then(r => r.json())
    .then(d => {
      const tAire = d.temp_aire;
      const ph    = d.ph;
      const niv   = d.nivel;
      const sal   = d.salinidad;

      // === Valores en tarjetas ===
      document.getElementById('temp_aire').textContent =
        (tAire == null || isNaN(tAire)) ? '--' : tAire.toFixed(1);

      document.getElementById('ph').textContent =
        (ph == null || isNaN(ph)) ? '--' : ph.toFixed(2);

      document.getElementById('nivel').textContent =
        (niv == null || isNaN(niv)) ? '--' : niv.toFixed(1);

      document.getElementById('salinidad').textContent =
        (sal == null || isNaN(sal)) ? '--' : sal.toFixed(2);

      // ============================================
      //   ESTADOS SEGÚN TABLA REAL DEL MANGLAR
      // ============================================

      // ----- TEMPERATURA AIRE -----
      const aireEstado = document.getElementById('estado_aire');
      aireEstado.classList.remove('metric-danger');

      if (tAire == null || isNaN(tAire)) {
        aireEstado.textContent = 'Sin dato';
      } else if (tAire <= 32) {
        aireEstado.textContent = 'Lectura OK';
      } else if (tAire <= 38) {
        aireEstado.textContent = 'Calor alto';
        aireEstado.classList.add('metric-danger');
      } else {
        aireEstado.textContent = 'Peligro';
        aireEstado.classList.add('metric-danger');
      }

      // ----- PH -----
      const phEstado = document.getElementById('estado_ph');
      phEstado.classList.remove('metric-danger');

      if (ph == null || isNaN(ph)) {
        phEstado.textContent = 'Sin dato';
      } else if (ph >= 6.5 && ph <= 8.5) {
        phEstado.textContent = 'Aceptable';
      } else {
        phEstado.textContent = 'Fuera de rango';
        phEstado.classList.add('metric-danger');
      }

      // ----- SALINIDAD -----
      const salEstado = document.getElementById('estado_sal');
      salEstado.classList.remove('metric-danger');

      if (sal == null || isNaN(sal)) {
        salEstado.textContent = 'Sin dato';
      } else if (sal >= 5 && sal <= 18) {
        salEstado.textContent = 'Aceptable';
      } else if (sal < 5) {
        salEstado.textContent = 'Muy baja';
        salEstado.classList.add('metric-danger');
      } else {
        salEstado.textContent = 'Muy alta';
        salEstado.classList.add('metric-danger');
      }

      // ----- NIVEL DE AGUA -----
      const nivelEstado = document.getElementById('estado_nivel');
      nivelEstado.classList.remove('metric-danger');

      if (niv == null || isNaN(niv)) {
        nivelEstado.textContent = 'Sin dato';
      }
      else if (niv > 90) {
        nivelEstado.textContent = 'Marea baja';
      }
      else if (niv > 60) {
        nivelEstado.textContent = 'Nivel aceptable';
      }
      else if (niv >= 40) {
        nivelEstado.textContent = 'Alerta leve';
      }
      else if (niv >= 20) {
        nivelEstado.textContent = 'Riesgo inundación';
        nivelEstado.classList.add('metric-danger');
      }
      else {
        nivelEstado.textContent = 'Alerta roja';
        nivelEstado.classList.add('metric-danger');
      }

      document.getElementById('last_update').textContent = formatearHora(new Date());

      // === Actualización gráfica ===
      if (niv != null && !isNaN(niv)) {
        nivelData.push(niv);
        timeLabels.push(new Date());

        if (nivelData.length > MAX_POINTS){
          nivelData.shift();
          timeLabels.shift();
        }
        dibujarGrafica();
      }
    })
    .catch(err => {
      console.log('Error al actualizar', err);
      document.getElementById('last_update').textContent = 'Error de conexión';
    });
}

function dibujarGrafica(){
  const canvas = document.getElementById('nivelChart');
  if (!canvas) return;

  const ctx = canvas.getContext('2d');
  const w = canvas.width;
  const h = canvas.height;

  ctx.fillStyle = '#020617';
  ctx.fillRect(0,0,w,h);

  if (nivelData.length < 2){
    ctx.fillStyle = '#9ca3af';
    ctx.font = '12px system-ui';
    ctx.fillText('Esperando más lecturas...', 20, h/2);
    return;
  }

  const margin = 30;
  const plotW = w - margin*2;
  const plotH = h - margin*2;

  const minVal = Math.min(...nivelData);
  const maxVal = Math.max(...nivelData);
  const rango = (maxVal === minVal) ? 1 : (maxVal - minVal);

  // Rejilla
  ctx.strokeStyle = '#1f2937';
  ctx.lineWidth = 1;
  ctx.beginPath();
  for (let i=0;i<=4;i++){
    const y = margin + (i/4)*plotH;
    ctx.moveTo(margin,y);
    ctx.lineTo(margin+plotW,y);
  }
  ctx.stroke();

  // Ejes
  ctx.strokeStyle = '#4b5563';
  ctx.beginPath();
  ctx.moveTo(margin,margin);
  ctx.lineTo(margin,margin+plotH);
  ctx.lineTo(margin+plotW,margin+plotH);
  ctx.stroke();

  // Línea
  ctx.strokeStyle = '#00c896';
  ctx.lineWidth = 2;
  ctx.beginPath();
  nivelData.forEach((v,i)=>{
    const x = margin + (i/(nivelData.length-1))*plotW;
    const y = margin + (1 - ((v - minVal)/rango))*plotH;
    if (i===0) ctx.moveTo(x,y);
    else ctx.lineTo(x,y);
  });
  ctx.stroke();

  // Puntos
  ctx.fillStyle = '#22c55e';
  nivelData.forEach((v,i)=>{
    const x = margin + (i/(nivelData.length-1))*plotW;
    const y = margin + (1 - ((v - minVal)/rango))*plotH;
    ctx.beginPath();
    ctx.arc(x,y,2.2,0,Math.PI*2);
    ctx.fill();
  });

  ctx.fillStyle = '#9ca3af';
  ctx.font = '11px system-ui';
  ctx.fillText(maxVal.toFixed(1)+' cm', 4, margin+4);
  ctx.fillText(minVal.toFixed(1)+' cm', 4, margin+plotH);

  ctx.fillText(formatearHora(timeLabels[0]), margin, margin+plotH+16);
  ctx.fillText(formatearHora(timeLabels[timeLabels.length-1]), margin+plotW-60, margin+plotH+16);
}

function toggleAuto(){
  const chk = document.getElementById('auto_chk');
  if (chk.checked){
    actualizar();
    autoTimer = setInterval(actualizar, 10000);
  } else {
    clearInterval(autoTimer);
  }
}

actualizar();
</script>





</body>
</html>
)rawliteral";

// ===================== PROTOTIPOS =====================

float leerTempAgua();
float leerDistancia();
float leerPH();
float leerEC25();
float calcularSalinidad(float ec25);
bool  leerAire(float &t, float &h);

void  sonarMelodia1();
void  mostrarPantallaDatos(float sal, float nivel, float ph);
void  mostrarPantallaLogo();

void  manejarRoot();
void  manejarDatos();

// ===================== SETUP =====================

void setup() {
  Serial.begin(115200);
  delay(500);

  ds18b20.begin();
  dht.begin();

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  pinMode(LED_VERDE, OUTPUT);
  pinMode(LED_ROJO, OUTPUT);
  pinMode(FAN_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  digitalWrite(LED_VERDE, HIGH);
  digitalWrite(LED_ROJO, LOW);
  digitalWrite(FAN_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  // ADC para pH y EC
  analogReadResolution(12);             // 0–4095
  analogSetPinAttenuation(PH_PIN,  ADC_11db);
  analogSetPinAttenuation(TDS_PIN, ADC_11db);

  // LCD
  Wire.begin(21, 22); // SDA = 21, SCL = 22 en ESP32
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("  ManglarLab");
  lcd.setCursor(0, 1);
  lcd.print(" Prototipo UPCH");
  delay(1500);

  // ===================== WIFI + SERVIDOR =====================

  Serial.println("\nIniciando WiFi (hotspot)...");

  WiFi.begin(ssid, password);              
  Serial.print("Conectando a WiFi: ");
  Serial.println(ssid);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\n✔ WiFi conectado");
  Serial.print("IP del ESP32: ");
  Serial.println(WiFi.localIP());

  // Página principal (HTML bonito)
  server.on("/", manejarRoot);

  // JSON para App Inventor y para la web (/datos y /json son lo mismo)
  server.on("/datos", manejarDatos);
  server.on("/json",  manejarDatos);

  server.begin();
  Serial.println("Servidor web iniciado");

  // Primera pantalla de datos (valores ficticios)
  mostrarPantallaDatos(0.0, 0.0, 7.0);
  showDataScreen = true;
  lastScreenChange = millis();
}

// ===================== LOOP =====================

void loop() {
  // Atender peticiones HTTP
  server.handleClient();

  // 1) Lecturas
  float tAgua = leerTempAgua();
  float nivel = leerDistancia();
  float ph    = leerPH();
  float ec25  = leerEC25();                  // EC corregida a 25°C (µS/cm)
  float sal   = calcularSalinidad(ec25);     // Salinidad en ppt

  float tAire = 0, hAire = 0;
  bool aireOk = leerAire(tAire, hAire);

  // Guardamos para el JSON
  if (aireOk) {
    ultimo_tAire = tAire;
    ultimo_hAire = hAire;
  } else {
    ultimo_tAire = NAN;
    ultimo_hAire = NAN;
  }
  ultimo_nivel = nivel;
  ultimo_ph    = ph;
  ultimo_sal   = sal;

  // 2) LEDs + buzzer por temperatura del agua (> 28 °C)
  if (tAgua > 28.0) {
    digitalWrite(LED_ROJO, HIGH);
    digitalWrite(LED_VERDE, LOW);
    sonarMelodia1();   // melodía que te gustó
  } else {
    digitalWrite(LED_ROJO, LOW);
    digitalWrite(LED_VERDE, HIGH);
    noTone(BUZZER_PIN);
    digitalWrite(BUZZER_PIN, LOW);
  }

  // 3) Ventilador por temperatura de aire (> 26 °C)
  if (aireOk && tAire > 26.0) {
    digitalWrite(FAN_PIN, HIGH);
  } else {
    digitalWrite(FAN_PIN, LOW);
  }

  // 4) Salida por Serial (para pruebas)
  Serial.println("========== ManglarLab - Lecturas ==========");

  Serial.print("Temp agua: ");
  Serial.print(tAgua, 2);
  Serial.print(" °C   |  ");

  Serial.print("Temp aire: ");
  if (aireOk) {
    Serial.print(tAire, 2);
    Serial.print(" °C   Hum: ");
    Serial.print(hAire, 1);
    Serial.print(" %");
  } else {
    Serial.print("ERROR DHT");
  }
  Serial.println();

  Serial.print("pH: ");
  Serial.print(ph, 2);
  Serial.print("   |  EC25: ");
  Serial.print(ec25, 1);
  Serial.print(" uS/cm   |  Sal: ");
  Serial.print(sal, 2);
  Serial.println(" ppt");

  Serial.print("Nivel: ");
  Serial.print(nivel, 2);
  Serial.println(" cm");

  Serial.print("LED verde: ");
  Serial.print(digitalRead(LED_VERDE) ? "ON" : "OFF");
  Serial.print("  | LED rojo: ");
  Serial.print(digitalRead(LED_ROJO) ? "ON" : "OFF");
  Serial.print("  | FAN: ");
  Serial.print(digitalRead(FAN_PIN) ? "ON" : "OFF");
  Serial.print("  | Buzzer: ");
  Serial.println(digitalRead(BUZZER_PIN) ? "ON" : "OFF");

  Serial.println("===========================================\n");

  // 5) LCD: alterna entre datos (10s) y logo (3s)
  unsigned long ahora = millis();

  if (showDataScreen) {
    mostrarPantallaDatos(sal, nivel, ph);

    if (ahora - lastScreenChange >= 10000UL) { // 10 s de datos
      mostrarPantallaLogo();
      showDataScreen = false;
      lastScreenChange = ahora;
    }

  } else {
    if (ahora - lastScreenChange >= 3000UL) {  // 3 s de logo
      mostrarPantallaDatos(sal, nivel, ph);
      showDataScreen = true;
      lastScreenChange = ahora;
    }
  }

  delay(1000);
}

// ===================== FUNCIONES DE SENSORES =====================

// --- Temperatura de agua (DS18B20) ---
float leerTempAgua() {
  ds18b20.requestTemperatures();
  float t = ds18b20.getTempCByIndex(0);
  if (t != DEVICE_DISCONNECTED_C) {
    tempAguaC = t;
  }
  return tempAguaC;
}

// --- Nivel de agua (HC-SR04) ---
float leerDistancia() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(5);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long dur = pulseIn(ECHO_PIN, HIGH, 30000);   // timeout 30 ms
  if (dur > 0) {
    ultimaDistancia = (dur * 0.0343f) / 2.0f;
  }
  return ultimaDistancia;  // cm (o -1 si nunca hubo lectura buena)
}

// --- pH (muy simple, luego lo calibras con buffers) ---
float leerPH() {
  long suma = 0;
  for (int i = 0; i < 10; i++) {
    suma += analogRead(PH_PIN);
    delay(5);
  }
  int adc = suma / 10;
  float volt = adc * (3.3f / 4095.0f);

  // Fórmula aproximada. Ajusta pendiente y offset con tus buffers.
  float ph = 7.0f + ((2.5f - volt) * 3.0f);
  return ph;
}

// --- EC25: conductividad corregida a 25°C ---
float leerEC25() {
  const int N = 10;
  long suma = 0;
  for (int i = 0; i < N; i++) {
    suma += analogRead(TDS_PIN);
    delay(5);
  }
  int adc = suma / N;

  // ADC -> voltaje (asumiendo Vref ~3.3V)
  float volt = adc * (3.3f / 4095.0f);

  // EC_medida a la temperatura actual (polinomio en voltios)
  float ec_medida = (133.42f * volt * volt * volt)
                  - (255.86f * volt * volt)
                  + (857.39f * volt);       // uS/cm aprox

  // Corrección a 25°C
  float ec25 = ec_medida / (1.0f + 0.02f * (tempAguaC - 25.0f));

  if (ec25 < 0) ec25 = 0;
  return ec25;
}

// --- Salinidad en ppt a partir de EC25 (µS/cm) ---
float calcularSalinidad(float ec25) {
  if (ec25 <= 0) return 0;
  float sal = 0.012f * pow(ec25, 0.89f);
  return sal;
}

// --- Aire (DHT11) ---
bool leerAire(float &t, float &h) {
  h = dht.readHumidity();
  t = dht.readTemperature();
  if (isnan(h) || isnan(t)) {
    return false;
  }
  return true;
}

// ===================== BUZZER (TU MELODÍA) =====================

void sonarMelodia1() {
  int tempo = 120;

  tone(BUZZER_PIN, 1000, tempo);
  delay(tempo + 30);

  tone(BUZZER_PIN, 1300, tempo);
  delay(tempo + 30);

  tone(BUZZER_PIN, 1500, tempo * 2);
  delay(tempo * 2 + 30);

  noTone(BUZZER_PIN);
}

// ===================== LCD =====================

void mostrarPantallaDatos(float sal, float nivel, float ph) {
  lcd.clear();

  // Fila 0: Salinidad
  lcd.setCursor(0, 0);
  lcd.print("Sal:");
  lcd.print(sal, 1);   // 1 decimal
  lcd.print("ppt");

  // Fila 1: pH y Nivel
  lcd.setCursor(0, 1);
  lcd.print("pH:");
  lcd.print(ph, 1);
  lcd.print(" Nv:");
  if (nivel >= 0) {
    lcd.print((int)nivel);
    lcd.print("cm");
  } else {
    lcd.print("--");
  }
}

void mostrarPantallaLogo() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("  ManglarLab");
  lcd.setCursor(0, 1);
  lcd.print(" Prototipo UPCH");
}

// ===================== HTTP HANDLERS =====================

// Página principal: HTML
void manejarRoot() {
  server.send_P(200, "text/html", MAIN_page);
}

// /datos y /json -> JSON para MIT App Inventor y panel web
void manejarDatos() {
  String json = "{";

  // temp_aire
  json += "\"temp_aire\":";
  if (isnan(ultimo_tAire)) json += "null";
  else json += String(ultimo_tAire, 1);
  json += ",";

  // ph
  json += "\"ph\":";
  if (isnan(ultimo_ph)) json += "null";
  else json += String(ultimo_ph, 2);
  json += ",";

  // nivel
  json += "\"nivel\":";
  if (isnan(ultimo_nivel)) json += "null";
  else json += String(ultimo_nivel, 1);
  json += ",";

  // salinidad
  json += "\"salinidad\":";
  if (isnan(ultimo_sal)) json += "null";
  else json += String(ultimo_sal, 2);

  json += "}";

  server.send(200, "application/json", json);
}