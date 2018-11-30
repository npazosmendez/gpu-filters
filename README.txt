Trabajo final para "Organización del Computador II", DC UBA

Filtros de Imágenes en OpenCL, para GPU
	- Detección de bordes (Canny)
	- Detección de líneas (Hough)
	- Estimación de optical flow (Lucas-Kanade)
	- Remoción de objetos / Inpainting

Pazos Méndez, Nicolás Javier
Reyes Mesarra, Darío René

Diciembre 2018

+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
########################### DEPENDENCIAS ##############################
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

* OpenCL 1.2 (lib)
  ~~~~~~
	sudo apt install ocl-icd-libopencl1
	sudo apt-get install ocl-icd-opencl-dev
	sudo apt install opencl-headers

* OpenCL (drivers y runtime)
  ~~~~~~
 	Se necesita un device que soporte OpenCL (cualquier GPU) con sus
 	drivers y su "OpenCL runtime" instalados.
 	Para las placas NVIDIA, lo necesario está incluido en los drivers:
 	https://developer.nvidia.com/opencl

* OpenCV 2.4.9 (para levantar imágenes/videos)
  ~~~~~~
	sudo apt-get install libopencv-dev

* Qt 4.8 (interfaz gráfica realtime)
  ~~~~~~
	sudo apt install qt4-default

+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
################################ USO ##################################
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Hay tres aplicaciones que pueden correrse. Todos los binarios se crean
en el directorio raíz, y **deben ser ejecutados desde allí** (se leen
los archivos *.cl en tiempo de ejecución).

* inpainter
  ~~~~~~~~~
  	Aplicación gráfica para enmascarar y remover partes de una imagen
	con el filtro de 'Inpainting'.
	La ruta de la imagen se pasa por consola, para info específica
	correr `./inpainter --help`

	- Compilación
		cd inpainter_app/
		make

* timer
  ~~~~~
  	Toma mediciones de los filtros en sus distintas implementaciones.
  	Para info específica correr `./timer --help`

	- Compilación
		cd timer_app/
		make

* realtime
  ~~~~~~~~
  	Aplicación gráfica interactiva para correr los filtros de Canny,
  	Hough y Lucas-Kanade en tiempo real para videos o para la webcam.

	- Compilación
		cd realtime_app/
		qmake
		make -j4

	NOTA: ante algún error de compilación/linkeo para esta app,
	se recomienda un `make clean` antes de reintentar.

+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
####################### ORGANIZACIÓN DEL CÓDIGO #######################
+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

* c/
	Los fuentes que implementan los filtros en C

* opencl/
	Los fuentes que implementan los filtros usando OpenCL.
	Acá también se encuentran los kernels con extensión .cl

* realtime_app/
	La aplicación que otorga una interfaz gráfica para correr los
	filtros, hecha en Qt.

* inpainter_app/
	La aplicación gráfica para correr el inpainter.

* timer_app/
	La aplicación para obtener mediciones comparativas entre las
	implementaciones C vs OpenCL de todos los filtros.
