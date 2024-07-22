# FIchier_E01


Tutoriel pour installer Docker sur Windows et utilisation du script permettant de lister les fichiers EO1

A. Construire le service Docker et télécharger le script 
Étape 1 : Installation de Docker Desktop sur Windows

		1.	Télécharger Docker Desktop :
			•    Allez sur le site officiel de Docker.
			•   Téléchargez et installez Docker Desktop pour Windows.
	
		2.	Lancer Docker Desktop :
			•   Après l’installation, ouvrez Docker Desktop et suivez les instructions pour terminer la configuration.
	
		3.	Vérifier l’installation (facultatif)
			•   Ouvrez le terminal (CMD) et tapez :   docker --version
					
					
Étape 2 : Copier les fichiers du dépôt GitHub
		1. Télécharger les données du script :
			· Allez sur la page de votre dépôt GitHub https://github.com/fabrice192/FIchier_E01
			· Copiez le contenu de chaque fichier et créez les fichiers correspondants sur votre machine Windows ( de préférence sur le bureau)
		
	
Étape 3 : Construire l’image Docker

		1. construire le service docker d'analyse de fichier E01
			· Taper dans le terminal : cd chemin\vers\votre\repertoire

		2. construire le service docker (besoin d'internet, cela peut prendre 5 a 10 min pour construire ce service)
			· Taper dans le terminal :   docker-compose up -d
		
B. Utilisation du service docker 
 
		1. Déposer les fichiers dans le bon répertoire DATA
			· Coller les fichiers E01 dans : chemin\vers\votre\repertoire\data


		2. Lancer docker (application)
		


		3. Dans le terminal : 
			· Taper dans le terminal : docker exec -it lister /bin/bash



	Cela permet de créer un terminal dans le service docker :

	Last login: Sun Jul 21 21:19:29 on ttys000
	fabrices.:_"@fabrice ~ % docker exec -it lister /bin/bash
	root@2d3de05136bd:/app/data#


		4. Lancer le script --> dans le terminal nouvellement créé par docker 
			·  Taper dans le terminal docker  : gcc -o list_fichier main.c -lewf -ltsk -lpthread
			·  Taper dans le terminal docker  : ./list_fichier ./data/XXXXXX.E01
		
			XXXXXX.E01 étant le nom du fichier EO1

		5. Le résultat de l'analyse apparait dans le fichier file_list.txt
			· Le RESULAT (file_list.txt) se trouve dans c  chemin\vers\votre\repertoire\


C. Des commandes à connaitre

Pour stopper le service docker , dans un terminal : docker stop lister
Pour supprimer définitivement : le service docker,  dans un terminal : system prune -a

Cette commande permet  compiler le fichier en c
 gcc -o list_fichier main.c -lewf -ltsk -lpthread

Cette commande permet  de lancer le script
 ./list_fichier ./data/LOUDD03.E01

