03/12/2018

Último commit realizado el 02/12/2018 dice que los errores de violación de segmento han sido arreglados. UPDATE: Siguen estando los errores.

FUNCIONALIDADES DE NIVEL IMPLEMENTADAS:

	-Diferenciación entre primer plano y segundo plano (comandos finalizados en &)
	-Mostrar lista de trabajos mediante comando interno jobs
	-Detener proceso en foreground cuando se presione Ctrl+Z

FUNCIONALIDADES POR IMPLEMENTAR:

	-Gestión correcta y completa de la lista de procesos y su estado.
	-Enviar un proceso detenido al foreground reactivando su ejecución, o uno del background al foreground mediante el comando interno fg.
	-Reactivar un proceso detenido para que siga ejecutándose pero en segundo plano, mediante el comando interno bg.
	
01/12/2018

En este nivel se implementarán las funcionalidades de los comandos internos internal_jobs(), bg(), y fg(). Además, se implementa el manejo del Ctrl+Z.

Además de eso, se tendrá que gestionar una lista de trabajos y su estado ('E' Ejecución, 'D' Detenido). Mostrar una lista de los trabajos existentes mediante el comando interno jobs. Diferenciar entre ejecución en primer plano y en segundo plano (Finalizados en &).

30/11/2018

A QUIEN PUEDA INTERESAR:

Este es el repositorio para el proyecto de la segunda práctica de la asignatura sistemas operativos I.

El siguiente código fue realizado por estudiantes de segundo año de ingeniería informática en la Universidad de las Islas Baleares.

El objetivo del proyecto es emular el funcionamiento del Shell Bash implementando funcionalidades como:

	-Comandos internos
	-Comandos externos
	-Tratamiento de señales
	-Llamadas de sistema de C

