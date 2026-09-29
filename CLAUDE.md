# Contenedores inteligentes: contexto del proyecto

## Qué es
Proyecto integrador de la materia **Ciudades inteligentes** (Ingeniería en Informática).
Problema urbano: contenedores de basura que se desbordan porque la recolección
sigue rutas fijas, sin saber qué contenedor está lleno. Propuesta: medir el
nivel de llenado de cada contenedor y recolectar según la demanda real.

El sistema tiene dos partes:
1. **Simulador en Wokwi** (fuera de este repositorio): ESP32 + sensor
   ultrasónico HC-SR04 que mide la distancia hasta la basura y calcula el
   porcentaje de llenado. Incluye un potenciómetro en GPIO 34 que simula la
   tensión de la batería.
2. **Panel web** (este repositorio): `index.html`, publicado con GitHub Pages
   en https://joacomantel.github.io/contenedores/

## Criterios de la materia (importantes para cualquier cambio)
- **Primero el problema, después la tecnología.** Cada función del panel tiene
  que servir a una decisión: cuándo y por dónde recolectar, qué mantener.
- **Regla de evidencia:** un dispositivo instalado es un producto; una mejora
  demostrada con indicadores es un resultado. El panel tiene que ayudar a
  mostrar resultados: km evitados, desbordes evitados, etc.
- **Honestidad técnica:** todo dato simulado o de ejemplo tiene que estar
  indicado como tal en la interfaz. No presentar estimaciones como mediciones.
- **Resiliencia y dependencia de proveedores:** si un servicio externo falla,
  el panel debe seguir funcionando en modo reducido, nunca quedar en blanco.
- **Privacidad:** el sistema no recolecta datos personales; mantenerlo así.

## Restricciones técnicas
- **Todo gratis, sin claves ni tarjeta.** Se descartó Google Maps por eso.
- **Un solo archivo** `index.html` con HTML, CSS y JS adentro. Librerías
  externas solo por CDN (hoy: Leaflet 1.9.4 desde cdnjs).
- Mapa real: **Leaflet + OpenStreetMap**. OSM solo entrega el mapa a páginas
  publicadas; si el archivo se abre local (`file://`) o el mapa no carga,
  el panel pasa automáticamente a un **mapa esquemático** en SVG.
- Ruta por las calles: **OSRM**, con el servidor público de demostración
  (`router.project-osrm.org`, servicio `trip`). Es gratuito pero para pruebas,
  con un límite de ~1 pedido por segundo. Si falla, se muestra un trazado
  aproximado en L. Para un piloto real haría falta un servidor OSRM propio.
- Textos de la interfaz en **español rioplatense** (voseo).
- Paleta ya definida en variables CSS (`--ink`, `--paper`, `--accent`, etc.):
  mantenerla.

## Qué tiene hoy el panel
- 14 contenedores de ejemplo con `lat`, `lng`, `llenado` (%), `ritmo`
  (% por hora) y `bateria` (%). Las coordenadas son de ejemplo, en el centro
  de Buenos Aires.
- Estados de llenado: bajo (<50%), medio (50–79%) y **alerta (≥80%)**.
- Lista lateral con filtros (Todos, En alerta, Medio, Bajo, Batería baja).
- **Ficha de detalle** por contenedor: dibujo del contenedor con la distancia
  que mide el sensor, datos, gráfico de 24 h (simulado) y parada en la ruta.
- **Ruta de recolección**: sale del depósito y vuelve. Incluye los contenedores
  en alerta y los que llegan a la alerta en las próximas 6 h. Muestra km,
  tiempo de manejo y el ahorro frente a pasar por los 14.
- **Panel de estadísticas**: indicadores, distribución por estado, llenado por
  contenedor, promedio de 24 h (simulado), baterías y orden sugerido.
- **Vista para vecinos** (formato app de celular): botón "Vista vecinos" o
  dirección `?vecinos` (sirve para un QR en cada contenedor). Mapa, "Cerca mío"
  (ordena por distancia con la ubicación del celular, que no sale del
  navegador), cómo llegar a pie (OpenStreetMap), reportar un problema y
  calificar con estrellas. Los reportes "Está lleno" y "Hay basura afuera"
  suman el contenedor a la ruta; "roto", "tapa", "sucio" quedan como
  mantenimiento. El municipio los ve en la ficha y los marca como resueltos.
  Limitación de la demo: reportes y calificaciones se guardan solo en el
  navegador (localStorage); hay 2 reportes y calificaciones de ejemplo.
- **Batería**: umbral de batería baja en 20%. Autonomía estimada con una 18650
  (~2400 mAh útiles) y un consumo de ~15 mAh/día (deep sleep + un envío cada
  15 min), o sea ~160 días por carga.

## Valores que deben coincidir con el simulador de Wokwi
- Altura del contenedor: **40 cm** (llenado = 100 · (1 − distancia / 40)).
- Umbral de alerta: **80%**, con histéresis de 3 puntos (se apaga bajo 77%).
- Batería: de 3,0 V (0%) a 4,2 V (100%), con una curva no lineal por tabla.
  Batería baja por debajo de 20%.

## Ideas y próximos pasos
1. ~~Conectar Wokwi con ThingSpeak~~ **(hecho)**: canal público **3486377**.
   El ESP32 envía el llenado (%) en `field1` y la batería (%) en `field2`,
   cada 20 s en la simulación y cada 15 min en el equipo real. El
   Contenedor 01 del panel muestra esos datos en vivo (constante
   `THINGSPEAK` en `index.html`, o `?canal=N` en la dirección). Lectura:
   `https://api.thingspeak.com/channels/3486377/feeds.json?minutes=1440&results=8000`.
   Si ThingSpeak falla, ese contenedor vuelve a sus datos de ejemplo.
2. **Comparación "antes vs. después"**: línea de base de ruta fija (el camión
   pasa por todos los contenedores) contra la recolección por demanda.
   Indicadores con definición, unidad, fuente y periodicidad: km recorridos,
   paradas por día, contenedores desbordados.
3. **Estado del sensor y calidad del dato**: marcar contenedores sin lectura
   reciente o con falla del sensor (el código de Wokwi ya contempla "SIN DATO").
4. ~~Reportes de vecinos~~ **(hecho, versión demo)**: falta un servidor para
   que los reportes le lleguen de verdad al municipio.
5. Posible: exportar datos a CSV y ajustar la accesibilidad.

## Forma de trabajo
- Antes de hacer cambios, **actualizá tu copia desde main**: a veces se suben
  archivos a mano desde la web de GitHub.
- Subí los cambios **directamente a la rama main**, así GitHub Pages se
  actualiza solo.
- Explicá cada cambio en lenguaje simple: soy estudiante y lo tengo que poder
  defender ante el profesor.
- No rompas lo que ya funciona: el respaldo al esquema, la ruta, la ficha de
  detalle, las estadísticas y la batería.
