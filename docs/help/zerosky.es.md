# Ayuda de ZeroSky

La vista del cielo sigue tu posición y la hora. En el dispositivo, también puede seguir la IMU. H abre esta página, F/X la desplazan y Esc la cierra.

## Páginas y movimiento

- 4 recorre Cielo, Vista, Cám y Conf. Mantén pulsado 4 para volver a la página anterior.
- En el dispositivo, TAB alterna entre el guiado por IMU y la cámara manual. En el escritorio el control es siempre manual.
- Fuera de Alinear, las flechas mueven la vista manual 5 grados. En el modo solo inclinación, las flechas izquierda/derecha alinean el norte arbitrario del giroscopio con el cielo real. Arriba/abajo siguen la IMU. F/X/Z/C también funcionan como arriba/abajo/izquierda/derecha fuera de la entrada de texto.

## Página Cielo

- 5 abre Ubicación. Escribe una ciudad, una latitud y una longitud, o elige una posición GPS. Usa arriba/abajo y Enter para seleccionar. El diálogo Ubicación se abre automáticamente en el primer uso.
- 6 aleja el zoom y 7 lo acerca. Las dos teclas están desactivadas mientras la cámara está encendida. El zoom del cielo se guarda al salir de la app.
- 8, marcada con una lupa, abre Buscar. Escribe parte del nombre de un objeto, elige una fila y pulsa Enter. Durante la guía, 8 muestra una señal de stop: púlsala para detener la guía y los pitidos. Un anillo marca un objeto en pantalla. Una flecha en el borde señala un objeto fuera de la vista. Pitidos más rápidos y agudos indican que estás más cerca.

## Página Vista

- 5 alterna entre el tema rojo de noche y el tema azul de día.
- 6 recorre el filtro de luz diurna. Una media estrella (Dim) oculta las estrellas débiles de día. Una estrella completa (All) las muestra.
- 7 recorre las figuras de las constelaciones: ocultas, solo líneas y luego líneas con nombres latinos. El icono pasa de un ojo tachado a líneas unidas y luego a una lista de estrellas.
- 8 muestra u oculta los 605 símbolos y nombres de cielo profundo. Buscar puede localizar igualmente un objeto oculto. Las dos opciones de Vista se guardan entre arranques.

## Página Cám

- 5, marcada con una cámara o una cámara tachada, enciende o apaga el fondo de cámara en vivo. La primera imagen puede tardar unos segundos. Las líneas, las estrellas y las etiquetas siguen visibles sobre la foto.
- 6, marcada con una mira o una marca de verificación, entra en Alinear o lo termina. De noche, apunta a estrellas conocidas. Lleva una estrella dibujada sobre su imagen en la foto: derecha mueve el dibujo a la derecha, izquierda lo mueve a la izquierda, y arriba/abajo lo mueven de la misma forma. Haz coincidir primero una estrella cerca del centro. Luego usa 7 y 8 para estrechar o ampliar el campo de la lente hasta que coincida otra estrella cerca de un borde.
- En Alinear, cada flecha cambia el desplazamiento de apuntado 0,5 grados. 7 y 8 cambian el campo de la lente 1 grado. Pulsa 6 de nuevo en **Listo** para guardar, solo si has cambiado algo. Apagar la cámara o salir con Esc descarta los cambios de Alinear sin terminar.
- Hasta que guardes una alineación, la línea de estado muestra FOV estimado cuando la cámara está encendida fuera de Alinear. El campo inicial de la lente, 62,2 grados, es una estimación.
- El tema de noche/día, el filtro de luz diurna, la cámara encendida/apagada y el zoom del cielo se recuerdan entre arranques. El zoom del cielo se guarda al salir. **Listo** guarda la alineación de la lente por separado.

## Página Conf

- 5 elige el idioma: Auto, EN, IT, FR, DE, ES, JA, ZH. El icono de traducir indica Auto, que sigue el sistema. Reinicia ZeroSky para aplicar el nuevo idioma.
- 6 recorre el perfil de hardware magnético: una señal de prohibición para none, una antena para cap, un conector USB para USB GPS. Usa la configuración que tengas conectada realmente.
- 7 abre Acerca de, el código QR del manual completo y Créditos.
- 8, marcada con una brújula, abre a pantalla completa la calibración magnética para el perfil de hardware seleccionado. Al aire libre, lejos de objetos metálicos, mueve el dispositivo trazando un 8 mientras lo giras por todos sus lados. Ocho puntos muestran la cobertura de direcciones. La barra muestra el avance hacia 200 muestras independientes y el tiempo restante. La captura se detiene cuando cada octante tiene al menos 15 muestras, o tras 120 segundos. Una marca de verificación y dos tonos ascendentes indican que el ajuste está listo: Enter guarda, Esc cancela. Un tono grave y un motivo indican que el ajuste ha fallado.

## Idioma

- La interfaz, esta ayuda y el manual existen en inglés, italiano, francés, alemán, español, japonés y chino simplificado. Una fuente CJK del sistema completa el texto en japonés y chino.
- La página Conf guarda la elección en ~/.config/zeroradio/lang, un archivo para toda la suite ZeroRadio. Por ahora solo ZeroSky lo lee. ZERORADIO_LANG tiene prioridad sobre él. Con Auto deciden LC_ALL, LC_MESSAGES y LANG.

## Línea de estado

- Az y Alt son la dirección de la vista. FOV es el campo de visión horizontal. Después aparece el nombre de la ubicación.
- manual: el guiado por IMU está desactivado y las flechas controlan la cámara.
- ejes no medidos: guarda la correspondencia de ejes del dispositivo con zerosky_probe.
- sin IMU: no se encontró el sensor.
- solo inclinación: la altura y el alabeo siguen la IMU. El azimut sigue el giroscopio, pero el norte es arbitrario y puede derivar. Usa las flechas izquierda/derecha para alinearlo. Una calibración válida hecha al aire libre para el perfil de hardware seleccionado activa el guiado respecto al norte verdadero.
- brújula perturbada: el campo magnético medido no coincide con el campo esperado. La IMU puede seguir guiando la vista. Aléjate de objetos metálicos o aparatos electrónicos, o pulsa TAB para el control manual.

## Calibración

- Mide primero los ejes del sensor, incluido el magnetómetro. Guárdalos con zerosky_probe --save-frames.
- Ajusta una posición, selecciona el perfil de hardware en Conf: 6 y pulsa la brújula en Conf: 8 al aire libre, lejos de objetos metálicos. Mueve el dispositivo trazando un 8 mientras lo giras por todos sus lados. El diálogo muestra las muestras distintas y los octantes cubiertos. Se detiene con la cobertura completa o tras 120 segundos.
- Un ajuste correcto muestra el RMS y el campo WMM. Enter guarda. Esc cancela sin cambiar el perfil anterior. Si el ajuste falla, sigue el motivo que aparece y repite. El archivo docs/probe.md del repositorio del código fuente describe la medida de los ejes.
