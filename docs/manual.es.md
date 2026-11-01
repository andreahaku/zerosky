# Manual de usuario de ZeroSky

ZeroSky es un planetario de bolsillo para el M5Stack CardputerZero. Dibuja el cielo para la hora actual del dispositivo y la ubicación que elijas, con un tema rojo de noche y un tema azul de día. La IMU puede guiar la vista incluso antes de la calibración magnética, con una dirección de norte arbitraria. Después de la calibración, la brújula da un rumbo respecto al norte verdadero. También puedes explorar con las teclas o mostrar la imagen de la cámara en vivo bajo el cielo calculado.

## Primeros pasos

1. Abre ZeroSky desde el lanzador del CardputerZero. En el primer uso aparece el diálogo Ubicación. Elige una ciudad, escribe coordenadas o espera una posición GPS, y luego pulsa Enter.
2. Comprueba el reloj del dispositivo. Las posiciones del cielo usan la hora del sistema. Seleccionar una ubicación GPS no ajusta el reloj.
3. Usa las flechas, o F/X/Z/C, para explorar en modo manual. Pulsa 4 para recorrer las páginas Cielo, Vista, Cám y Conf de la barra inferior.
4. Para que la vista siga el dispositivo, mide y guarda los ejes de sus sensores. Sin calibración magnética, `solo inclinación` sigue la altura y el alabeo, y las flechas izquierda/derecha alinean el rumbo. Calibra el magnetómetro al aire libre para activar el guiado respecto al norte verdadero. La [guía de la sonda de campo](probe.md) da los comandos.

## Cómo leer el cielo

El centro de la pantalla es la dirección de apuntado actual. El azimut se mide en sentido horario desde el norte verdadero: 0° norte, 90° este, 180° sur y 270° oeste. La altura es 0° en el horizonte y positiva por encima de él. La línea de estado muestra `Az`, `Alt`, el `FOV` horizontal y la ubicación seleccionada.

El tamaño de las estrellas depende de su brillo. Las líneas unen las figuras de las constelaciones occidentales. Son ilustrativas y no corresponden a los límites oficiales de la IAU. Sus nombres latinos aparecen en puntos de etiqueta de d3-celestial. Los objetos de cielo profundo tienen símbolos de carta y nombres de catálogo. El Sol, la Luna y los planetas tienen etiquetas. El suelo oscuro y la línea del horizonte muestran dónde el cielo se encuentra con el plano horizontal local. La pantalla usa direcciones calculadas y un horizonte simple. Los edificios y el terreno cercanos no están modelados.

Con el filtro de luz diurna activado, el límite de estrellas cambia de forma gradual mientras el Sol desciende por los crepúsculos civil, náutico y astronómico. Es una ayuda de visualización, no una medida del brillo real del cielo local. Las nubes, las luces de la ciudad y la luz de la Luna pueden cambiar lo que ves realmente. Desactiva el filtro de luz diurna en la página Vista para mostrar el catálogo nocturno durante el día. Las líneas de las constelaciones y los objetos del sistema solar se siguen dibujando en la vista actual.

## Teclas

| Tecla | Página Cielo | Página Vista | Página Cám | Página Conf |
| --- | --- | --- | --- | --- |
| 4 | Página siguiente; mantén pulsada para la anterior | Página siguiente; mantén pulsada para la anterior | Página siguiente; mantén pulsada para la anterior | Página siguiente; mantén pulsada para la anterior |
| 5 | Abre Ubicación | Alterna entre los temas de noche y de día | Fondo de cámara encendido/apagado | Elige el idioma |
| 6 | Aleja el zoom (cámara apagada) | Cambia el filtro de luz diurna: media estrella (Dim) o estrella completa (All) | Alinear/Listo | Elige el hardware magnético `none`, `cap` o `usb_gps` |
| 7 | Acerca el zoom (cámara apagada) | Recorre las constelaciones: ocultas, líneas, líneas con nombres latinos | Estrecha el FOV de la lente en Alinear | Abre Acerca de, el QR del manual y Créditos |
| 8 | Abre Buscar (lupa); durante la guía, detiene (señal de stop) | Muestra/oculta los símbolos y nombres de cielo profundo | Amplía el FOV de la lente en Alinear | Abre la calibración de la brújula |

Fuera de Alinear, cada pulsación de una flecha gira la vista manual 5°. En `solo inclinación`, las flechas izquierda/derecha ajustan el desplazamiento del rumbo. La altura y el alabeo siguen la IMU. En el CardputerZero, F/X/Z/C también funcionan como arriba/abajo/izquierda/derecha fuera de un campo de texto. En el dispositivo, TAB alterna entre el guiado por IMU y el control manual. El simulador de escritorio es siempre manual. H abre una página de ayuda más corta. ESC cierra un diálogo o una página de ayuda abiertos, o sale de la app cuando no hay nada superpuesto. En la ayuda y en Acerca de, F/X desplazan el texto y Z/C cambian de página donde hay varias.

La página Cám usa un icono de cámara para la tecla **5** cuando la imagen está apagada y una cámara tachada cuando está encendida. La tecla **6** muestra una mira para Alinear y una marca de verificación para **Listo**. Los números de las teclas no cambian aunque cambie el icono.

En la página Vista, la tecla **7** muestra un ojo tachado cuando las constelaciones están ocultas, líneas unidas para las figuras solas y una lista de estrellas para las figuras con nombres latinos. La tecla **8** muestra círculos cuando los símbolos y nombres de cielo profundo están activos, o un círculo discontinuo cuando están desactivados. Los nombres y símbolos pueden omitirse donde se amontonarían con otra etiqueta. Por defecto están activas las líneas de constelaciones con nombres y el cielo profundo. Las dos opciones se guardan en `~/.config/zerosky/settings`. La lista de Buscar y su guía siguen incluyendo los objetos de cielo profundo cuando su visualización está desactivada.

Con la cámara apagada, el campo de visión horizontal va de 15° a 120°. Un campo más estrecho amplía el cielo y muestra estrellas más débiles del catálogo cuando el filtro de luz diurna lo permite. Las teclas de zoom del cielo están desactivadas mientras la cámara está encendida. El zoom del cielo se guarda al salir de la app, no después de cada pulsación.

## Cámara y alineación de la lente

En la página **Cám**, pulsa **5** para mostrar la imagen de la IMX219 detrás de las estrellas calculadas, las líneas de las constelaciones, el horizonte y la guía. La primera imagen puede tardar unos segundos. Las líneas y las etiquetas usan un contorno o un fondo oscuro para seguir siendo legibles sobre una escena luminosa. Si falta la cámara o su imagen deja de actualizarse, el fondo del cielo calculado sigue siendo utilizable. Pulsa **5** de nuevo para liberar la cámara. Mientras está encendida, el zoom del cielo está desactivado y la vista usa el FOV de la lente. El valor inicial, 62,2° en horizontal, es una estimación a partir de la Pi Camera v2, no una medida de esta unidad. Hasta que guardes una alineación, la línea de estado muestra `FOV estimado` cuando la cámara está encendida y Alinear está cerrado.

Mide la alineación de noche con dos o más estrellas conocidas repartidas por la imagen:

1. Elige una zona despejada del cielo con una estrella reconocible cerca del centro y otra cerca del borde. Ajusta la ubicación y la hora correctas, y luego enciende la cámara con **Cám: 5**. Una brújula calibrada da la dirección inicial más fiable.
2. Sujeta el dispositivo sin moverlo y pulsa **Cám: 6** para entrar en **Alinear**. Usa las flechas para llevar el cielo dibujado hacia la estrella fotografiada hasta que la estrella central coincida: derecha mueve el dibujo a la derecha, izquierda lo mueve a la izquierda, y arriba/abajo lo mueven en su dirección. Cada pulsación cambia el desplazamiento hacia la derecha o hacia arriba de la pantalla 0,5°. La línea de estado muestra los desplazamientos.
3. Mantén la estrella central alineada. Pulsa **Cám: 7** para estrechar o **Cám: 8** para ampliar el FOV de la lente en pasos de 1° hasta que la estrella cerca del borde también coincida. Vuelve a comprobar el centro y repite si hace falta.
4. Pulsa **Cám: 6** de nuevo cuando la tecla 6 muestre la marca de verificación (**Listo**). Si has cambiado un valor, se guardan el FOV y los desplazamientos de apuntado hacia la derecha y hacia arriba de la pantalla en `~/.config/zerosky/camera-alignment` (o `$XDG_CONFIG_HOME/zerosky/camera-alignment`). Estos desplazamientos solo se aplican mientras la cámara está encendida. **Listo** sin cambios no marca el FOV estimado como medido.

Para descartar los cambios sin terminar, apaga la cámara con **Cám: 5** o sal de la app con ESC mientras Alinear está abierto. Ninguna de las dos acciones guarda la alineación sin terminar. Cuando se ha guardado una alineación, `FOV estimado` desaparece de la línea de estado de la cámara.

La alineación de la cámara es independiente de la correspondencia de ejes de la IMU y de la calibración magnética. Corrige el campo y el centro de la imagen fotografiada. No corrige un reloj, una ubicación o una referencia de norte incorrectos. `ZEROSKY_CAMERA_FOV` sustituye el FOV guardado en una ejecución controlada, así que déjala sin definir mientras mides. La alineación óptica y la precisión de apuntado nocturna todavía no se han verificado en el dispositivo.

## Ajustes de vista guardados

ZeroSky restaura después de un reinicio el tema de noche/día elegido, el filtro de luz diurna (media estrella o estrella completa), la cámara encendida/apagada y el zoom normal del cielo. Estas opciones se guardan en `~/.config/zerosky/settings` (o `$XDG_CONFIG_HOME/zerosky/settings`). El zoom del cielo se guarda al salir de la app y no en cada pulsación de las teclas de zoom. La alineación de la lente de la cámara tiene su propio archivo, como se describe arriba. La elección del hardware magnético se guarda en `~/.config/zerosky/hardware` y el idioma en `~/.config/zeroradio/lang`. Las variables de entorno como `ZEROSKY_THEME`, `ZEROSKY_DAYLIGHT`, `ZEROSKY_CAMERA`, `ZEROSKY_CAMERA_FOV` y `ZEROSKY_HARDWARE` tienen prioridad cuando se definen al arrancar. Si guardaste la cámara encendida, la captura puede empezar automáticamente en el siguiente arranque.

## Idioma

La interfaz, la ayuda de la app y este manual existen en inglés, italiano, francés, alemán, español, japonés y chino simplificado. El texto en japonés y chino usa una fuente CJK de respaldo del sistema.

Pulsa **Conf: 5** para elegir el idioma. Cada pulsación pasa a la opción siguiente: Auto, EN, IT, FR, DE, ES, JA, ZH, y de nuevo Auto. La tecla muestra el código de dos letras, o un icono de traducir para Auto. Un aviso confirma el cambio. Reinicia ZeroSky para aplicarlo: hasta entonces la interfaz mantiene el idioma anterior. Auto sigue la configuración regional del sistema.

La elección se guarda en `~/.config/zeroradio/lang` (o `$XDG_CONFIG_HOME/zeroradio/lang`). El archivo está pensado para toda la suite ZeroRadio, pero por ahora solo ZeroSky lo lee. Al arrancar, ZeroSky lee primero `ZERORADIO_LANG`, luego este archivo y después `LC_ALL`, `LC_MESSAGES` y `LANG`. Las dos primeras letras eligen la traducción. Un idioma no disponible vuelve al inglés. El código QR de la página Acerca de abre este manual en el idioma seleccionado.

## Ubicación

Pulsa **Cielo: 5** para abrir Ubicación. Escribe el nombre de una ciudad de la lista de ciudades sin conexión o unas coordenadas como `36.05, 14.24` (latitud positiva al norte, longitud positiva al este). Muévete con arriba/abajo y pulsa Enter en la fila que quieras. Un receptor GPS USB o el Cap LoRa-1262-GPS pueden ofrecer una posición. La fila GPS solo se puede seleccionar cuando tiene una posición. Sin hardware GNSS puedes usar igualmente una ciudad o coordenadas escritas a mano.

ZeroSky guarda la posición seleccionada en el archivo de ubicación compartido con ZeroRadio, así las dos apps pueden usar el mismo lugar. La fuente de la ubicación y el ajuste de hardware magnético son independientes: elegir una ciudad no significa que el cap o el GPS USB no estén conectados. La posición, la fecha actual y el WMM2025 determinan la referencia magnética que usa la brújula.

## Buscar un objeto

Pulsa **Cielo: 8** para abrir Buscar. Escribe parte de un nombre o de una designación de catálogo, luego selecciona una fila y pulsa Enter. La lista incluye el Sol, la Luna, los planetas, las estrellas con nombre y los objetos Messier, NGC e IC incluidos en la app. Los objetos bajo el horizonte aparecen marcados. Durante la guía, 8 muestra una señal de stop: púlsala para detener la guía y los pitidos. Si el objetivo está en pantalla, lo marca un anillo. Si está fuera del campo, una flecha en el borde indica hacia dónde girar, y la etiqueta da la separación angular. Un objetivo bajo el horizonte se puede seleccionar, pero no emite pitidos. En los demás casos, la señal sonora se vuelve más rápida y aguda a medida que te acercas. A menos de 1° aproximadamente, se vuelve continua. El sonido necesita un dispositivo de audio que funcione.

La búsqueda usa las posiciones que calcula la app. Su precisión depende del reloj del dispositivo, de la ubicación, de la correspondencia de ejes y de la calibración magnética. Para elegir un objetivo al arrancar durante una demostración, `ZEROSKY_TARGET` puede indicar un objeto compatible.

## Apuntar con la IMU

El BMI270 mide la aceleración y la rotación. El BMM150 da una dirección magnética. ZeroSky las combina con un filtro Mahony y usa el World Magnetic Model 2025 para referir el rumbo al norte verdadero. Los ejes de los sensores del dispositivo y el eje que representa el centro de la pantalla se deben medir en la unidad física. Una matriz identidad de fábrica no demuestra la alineación.

Con una correspondencia de ejes válida, pero sin una calibración o una referencia magnética utilizables, el estado es `solo inclinación`. La altura y el alabeo siguen la IMU. El azimut sigue el giroscopio desde una dirección inicial arbitraria y puede derivar. Apunta a un objeto o a un punto de referencia conocido y usa las flechas izquierda/derecha para alinear la vista. Las flechas arriba/abajo no guían la vista mientras la IMU está activa. TAB pasa al control manual completo. Cuando la configuración de hardware seleccionada tiene una calibración válida hecha al aire libre y la brújula funciona bien, ZeroSky usa el campo magnético para guiar la vista respecto al norte verdadero y `solo inclinación` desaparece.

La configuración usa dos archivos separados en `~/.config/zerosky/` (o `$XDG_CONFIG_HOME/zerosky/`):

- `frame-mapping.json` registra las rotaciones verificadas del acelerómetro, del giroscopio y del magnetómetro, el eje de apuntado y el eje de la parte superior de la pantalla. Guarda el esquema 2 con `zerosky_probe --save-frames` y las cinco opciones de ejes, después de medir los ejes. Los archivos existentes con esquema 1 siguen cargándose. Si no tienen eje magnético, la app puede reutilizar el `sensor_to_body` medido de un perfil magnético existente.
- `magnetometer-calibration.json` guarda un ajuste hard-iron/soft-iron hecho al aire libre para cada configuración de hardware montada. Captúralo con `zerosky_probe --capture none`, `--capture cap` o `--capture usb_gps`, usando la posición actual, la fecha UTC y una rotación del magnetómetro verificada.

La sonda instalada es `/usr/share/zerosky/bin/zerosky_probe`. Ejecútala con `--help` para ver sus opciones y sigue [docs/probe.md](probe.md) para el procedimiento de medida. No copies las matrices de rotación de ejemplo de la guía, salvo que coincidan con los ejes medidos en tu dispositivo. La sonda rechaza un ajuste sin una cobertura tridimensional suficiente y conserva un perfil válido existente si el nuevo ajuste falla.

Para la calibración magnética en la app, ajusta primero una posición y selecciona en **Conf: 6** el perfil del hardware conectado físicamente. Al aire libre, lejos de objetos metálicos, pulsa el icono de la brújula en **Conf: 8**. Sigue el gran 8 animado mientras giras el dispositivo por todos sus lados, también boca abajo. Un 8 trazado en plano no cubre la esfera. Ocho puntos muestran los octantes cubiertos, y la barra muestra las muestras independientes y el tiempo restante. La captura se detiene con 200 muestras independientes y al menos 15 en cada octante, o tras 120 segundos. Dos tonos ascendentes anuncian un ajuste correcto. Un tono grave indica un fallo. El ajuste comprueba la cobertura tridimensional y el residuo sobre muestras reservadas. Después aparece una gran marca de verificación con el RMS y el campo WMM esperado. Pulsa Enter para guardar el perfil seleccionado y recargarlo al momento, o Esc para cancelar sin escribir nada. Un ajuste fallido indica un motivo, y el perfil anterior se conserva. Si no hay ejes magnéticos medidos en el esquema 2 ni en un perfil anterior, guarda primero los ejes con la sonda.

En la página **Conf**, la tecla 6 recorre `none` (una señal de prohibición), `cap` (una antena) y `usb_gps` (un conector USB). Elige la configuración física conectada realmente, aunque la posición venga de una ciudad guardada. La selección se guarda en `~/.config/zerosky/hardware`. `ZEROSKY_HARDWARE` selecciona otro valor al arrancar la app para ejecuciones controladas. La app nunca sustituye en silencio el perfil que falta para la configuración seleccionada por otro perfil. Mientras la IMU está disponible, usa `solo inclinación`. La detección automática de accesorios todavía no está activada.

## Mensajes de estado

| Mensaje | Significado y acción |
| --- | --- |
| `manual` | La cámara se controla con las teclas. TAB puede activar el guiado por IMU cuando está disponible. |
| `ejes no medidos` | No se cargó ninguna correspondencia de ejes válida. Verifica los ejes con la sonda y guarda la correspondencia. |
| `archivo de ejes no válido` | La correspondencia de ejes guardada no pasó la validación. Repite la comprobación de ejes y guarda un archivo válido. |
| `sin IMU` o `la IMU no arranca` | La app no pudo encontrar o iniciar los sensores. El control manual sigue disponible. |
| `solo inclinación` | La altura y el alabeo siguen la IMU. El rumbo del giroscopio tiene un norte arbitrario y puede derivar. Alinéalo con las flechas izquierda/derecha. Calibra al aire libre la configuración de hardware seleccionada para guiar la vista respecto al norte verdadero. |
| `brújula perturbada` | El campo medido no coincide con el campo esperado. El guiado por IMU puede continuar con un rumbo poco fiable. Aléjate de objetos metálicos o aparatos electrónicos cercanos, o pulsa TAB para el control manual. |

La app actualiza las posiciones del cielo de forma periódica. Una fecha o una ubicación inexactas desplazan los objetos previstos aunque la brújula funcione bien. Una calibración hecha junto a una barandilla metálica también puede pasar la validación del archivo y dar un rumbo malo en otro lugar. Compruébala al aire libre con una referencia independiente, celeste o geográfica.

## Datos, licencias y límites actuales

El catálogo estelar contiene 5070 estrellas de HYG v4.1 hasta la magnitud 6. La app aplica la precesión a sus posiciones J2000 hasta la fecha de observación, pero todavía no aplica el movimiento propio de cada estrella. Una selección derivada de OpenNGC añade 605 objetos Messier, NGC e IC. Astronomy Engine calcula el Sol, la Luna y los planetas. WMM2025 da la declinación magnética y la intensidad de campo esperada sin conexión. La lista de ciudades de GeoNames también funciona sin conexión.

El código de ZeroSky tiene licencia MIT. Los archivos derivados de estrellas HYG y de cielo profundo OpenNGC tienen licencia CC BY-SA 4.0. Las líneas de las constelaciones occidentales llevan los avisos BSD-3-Clause de d3-celestial y CC BY 4.0 de IAU / *Sky & Telescope*. El código y los coeficientes del WMM son material de dominio público del Gobierno de EE. UU. Consulta [CREDITS.md](../CREDITS.md) y las [fuentes de datos](data-sources.md) para las revisiones y la atribución.

El fondo de cámara y el altavoz funcionan en el CardputerZero. El perfil magnético `cap` se ha calibrado al aire libre, y se ha usado el Sol para una primera comprobación de la alineación. Los demás perfiles de hardware, la alineación de la lente y la precisión de apuntado nocturna todavía necesitan validación. La exposición larga y el plate solving no están implementados. El [README](../README.md) contiene las instrucciones de compilación y el estado del desarrollo.
