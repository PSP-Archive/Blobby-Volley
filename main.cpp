/*
Blobby Volley pour PSP

Un projet similaire avait deja vu le jour sur PSP il y a pas mal de temps. J'ai r?-?crit entierement le code et on peut jouer contre une IA (debile).
Les images utilisees dans le jeu sont prises du 1er projet sorti sur PSP qui viennent elles-memes du depot sourceforge de Blobby Volley Officiel: http://sourceforge.net/projects/blobby

Le code source est fourni comment? et peut servir de base a un debutant.
Je n'ai plus vraiment de temps a consacrer a ce projet, vous etes donc libre de modifier ce code et de l'ameliorer comme bon vous semble (un mode adhoc serait le bienvenue :) ainsi que plusieurs niveaux d'IA et du son!)..

Poustak 16/11/08

Projet Blobby Volley officiel: http://sourceforge.net/projects/blobby
*/

#include <pspkernel.h>
#include <pspdebug.h>
#include <pspctrl.h>
#include <pspsdk.h>
#include <pspdisplay.h>
#include <stddef.h>
#include <oslib/oslib.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <psputils.h>
#include <string.h>


// Parametres habituels pour PSP et OSLib
PSP_MODULE_INFO("Blobby Test", 0, 1, 1);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER | THREAD_ATTR_VFPU);

/* structures d'un blobby et du ballon */

typedef struct	{
	// Positions
	float x, y;
	// Taille du perso
	float width, height;
	//vitesse verticale
	float vy;
	//Pour savoir quelle image du blobby afficher 
	int frameNb;
	//Blobby au sol ou pas (sert a savoir si le Blobby peut sauter)
	bool auSol;
} BLOBBY;

typedef struct	{
	// Positions
	float x, y;
	// Taille du perso
	float width, height;
	//vitesse verticale
	float vx,vy;
	//Pour savoir quelle image du ballon afficher
	int frameNb;
	//le ballon est en mouvement ou pas (sert au moment du service)
	bool Moving;
} BALLON;



/*
	Constantes
*/
const float gravite = 0.25f;
//deplacement par pas de 5 pixels
const float move = 5.0f;
//vitesse de depart lors d'un saut
const float vitesseSaut = -6.5f;
// ca c'est quand le blobby il touche bien par terre ;)
const float niveauSol = 240.0f;

/*
	Images
*/
OSL_IMAGE *imgFond;
OSL_IMAGE *imgBlobbyR;
OSL_IMAGE *imgBlobbyB;
OSL_IMAGE *imgOmbre;
OSL_IMAGE *imgBall;
OSL_IMAGE *imgOmbreBall;
OSL_IMAGE *imgTitre;
OSL_IMAGE *imgFontMin;
OSL_IMAGE *imgFontMaj;
OSL_IMAGE *imgTrophee;
/* Fonction */

int game_main(int option);


/* Variables Globalles */
//Pour les calculs pour l'IA
static float delta, t; 
static float px;
static float collision_fillet;
//scores
static int score_b1 = 0;
static int score_b2 = 0;
static int avantage;
//pour les collisions
static int touch_b1 = 0;
static int touch_b2 = 0;
static float temps = 0.0f;
static float last_touch_b1 = 0.0f;
static float last_touch_b2 = 0.0f;



//G?re le Blobby Joueur gauche(appel? une fois par frame)
void GereBlobby1(BLOBBY &blobby)	
{
	
	//Gestion des frames
	blobby.frameNb ++;
	
	//D?placement horizontal
	if (osl_pad.pressed.left)	
	{
		blobby.x = blobby.x - move;
		if (blobby.x < -5)
			blobby.x = -5;
	}
	if (osl_pad.pressed.right)	{
		blobby.x = blobby.x + move;
		if (blobby.x > 200)
			blobby.x = 200;
	}
	//D?placement vertical + Gestion gravit? qui influe sur la vitesse (VY)
	blobby.y = blobby.y + blobby.vy;
	blobby.vy = blobby.vy + gravite;

	//Gestion des collisions avec le sol
	if (blobby.y + blobby.height >= niveauSol)		{
		//Au sol, la vitesse passe ? z?ro (et on re?oit le contre-coup, mais pas dans un jeu :p)
		blobby.vy = 0.0f;
		//Remet au niveau du sol
		blobby.y = niveauSol - blobby.height;
		
		//Il est au sol => il pourra sauter
		blobby.auSol = true;
	}

	//Croix: Saut
	if (osl_pad.pressed.cross && blobby.auSol)	{
		//Vitesse n?gative: vers le haut
		blobby.vy = vitesseSaut;
		blobby.auSol = false;
	}
}

//G?re le Blobby IA (appel? une fois par frame)
void GereBlobbyIA(BLOBBY &blobby, BALLON &ballon)	{
	

	delta = 0;
	px = 0;
	t = 0;
	int x = 0;	
	
	
	//Proch. ?tape d'anim, utilis?e pour l'animation du blobby
	blobby.frameNb ++;
	
	// Collision Fillet et extremite ecran
	if (blobby.x <= 244)
		blobby.x = 244;
	
	if ((blobby.x + blobby.width) >= 485)
		blobby.x = 485 - blobby.width;
		
	//D?placement vertical + Gestion gravit? qui influe sur la vitesse (VY)
	blobby.y = blobby.y + blobby.vy;
	blobby.vy = blobby.vy + gravite;

	//Gestion des collisions avec le sol
	if (blobby.y + blobby.height >= niveauSol)		{
		//Au sol, la vitesse passe ? z?ro
		blobby.vy = 0.0f;
		//Remet au niveau du sol
		blobby.y = niveauSol - blobby.height;
		
		//Il est au sol => il pourra sauter
		blobby.auSol = true;
	}
	
	//Prevision de la trajectoire du ballon et reaction IA
	if (((ballon.x + ballon.width) < 200) || (ballon.y + ballon.height >= niveauSol)){
		// repositionnement au centre		
		if ((blobby.x < 380) | (blobby.x > 385)){
			if (blobby.x < 380){
				blobby.x = blobby.x + move;
			}
			else {
				blobby.x = blobby.x - move;
				}
		}
	}
	else {

		/*	Un peu de Physique...
		 *      y = g * t  + vy  * t + y
		 *	y = lim_bas <=> g*t  + vy * t + (y - lim_bas) = 0
		 *	delta = vy  - 4 * g * y
		 *	t = (-vy + sqrt( vy  - 4 * g * (y - lim_bas)))/(2 * g)
		 *	x = vx  * t + x  */

		delta = ballon.vy - 4 * gravite * (ballon.y - niveauSol);
		t = (-ballon.vy + sqrt((unsigned int)(delta))) / (2.0 * gravite); 
		px = ballon.x + ballon.vx * t;
		px = (px <= 480 ? px : 2 * 480 - px); /* px = x pour lequel la balle va toucher le sol */
	
		//Pour le service, le blobby a besoin de sauter
		if (ballon.vx == 0 && ballon.vy == 0){
			if (blobby.auSol){
					blobby.vy = vitesseSaut;
					blobby.auSol = false;
				}
		}
		
		//pour que l'IA fasse un saut de temps en temps
		x= rand() % 100;
		if (x < 10)
		{
			//le blobby saute seulement si le ballon est assez haut et si il peux le toucher (enfin c'est de l'a peu pres ici)
			if (ballon.y < 127 && (fabs(ballon.x - blobby.x) < 40)){
				if (blobby.auSol){
					blobby.vy = vitesseSaut;
					blobby.auSol = false;
				}
				//si le ballon va atterrir devant le blobby
				if((px > 200) && (((px + 14) - (blobby.x + blobby.width / 2)) < -20)) { 
					blobby.x = blobby.x - move;
				} 
				//si le ballon va atterrir derriere le blobby
				else if((px > 200) && (((px + 14) - (blobby.x + blobby.width / 2)) > -8)){ 
					blobby.x = blobby.x + move;
				}
								
			}
		}
		else {
		// Placement du Blobby en fonction de la ou va tomber la balle sans sauter
			if((px > 200) && (((px + 14) - (blobby.x + blobby.width / 2)) < -14)) { 
				blobby.x = blobby.x - move;
			} 
			else if((px > 200) && (((px + 14) - (blobby.x + blobby.width / 2)) > -8)){ 
				blobby.x = blobby.x + move;
			}
		}
		
		
	}
		
}





//G?re le Ballon (appel? une fois par frame)
void GereBallon(BALLON &ballon, BLOBBY &blobby1, BLOBBY &blobby2)	{
	
	//Proch. ?tape d'anim, utilis?e pour l'animation du ballon
	ballon.frameNb ++;
	
	//Gestion Collisions avec le blobby1 (celui a gauche)
	if (((blobby1.x >= ballon.x) & (blobby1.x <= (ballon.x + ballon.width))) || (((blobby1.x + blobby1.width) >= ballon.x) & 
	    ((blobby1.x + blobby1.width) <= (ballon.x + ballon.width))) || (((blobby1.x + 17.5) >= ballon.x) & ((blobby1.x + 17.5) <= (ballon.x + ballon.width))))
	{
		if (((blobby1.y <= (ballon.y + ballon.height)) & (blobby1.y >= ballon.y)) || (((blobby1.y + blobby1.height) >= ballon.y) & 
		    ((blobby1.y + blobby1.height) <= (ballon.y + ballon.height))) || (((blobby1.y + 20) >= ballon.y) & ((blobby1.y + 20) <= (ballon.y + ballon.height))))
		{
			//calcul de la nouvelle vitesse du ballon apres collision
			ballon.vx = ((ballon.x + 14)-(blobby1.x + 17.5)) / 4; 
			ballon.vy = ((ballon.y + 14)-(blobby1.y + 20)) / 4; 
			
			//Les blobbys n'on droint qu'a 3 touches consecutives
			//cette variable temps permets de ne compter qu'une touche par tranche de 25 frames
			if (temps - last_touch_b1 > 25)
				touch_b1 ++;
			
			last_touch_b1 = temps;
			
			//blobby2 n'est pas le dernier a avoir touche la balle, on remet son compteur a 0
			touch_b2 = 0;
			
			//Collision avec le fillet possible
			collision_fillet = 1;
			
			//ici on gere le cas ou le ballon est a la verticla parfaite du blobby, on lui donne une legere vitesse horizontale et un vitesse verticale suffisante
			if (((ballon.x + 14)-(blobby1.x + 17.5)) == 0.00){
				switch (rand() %2){
					case 0:
						ballon.vx = 1;
						break;
					case 1:
						ballon.vx = -1;
						break;
				} 
				ballon.vy = -3;
			}
			
			//limite de vitesse du ballon, sinon c'est du flipper ;)
			if (ballon.vx > 13)
				ballon.vx = 13;
			if (ballon.vx < -13)
				ballon.vx = -13;
					
			if (ballon.vy > 13)
				ballon.vy = 13;
			if (ballon.vy < -13)
				ballon.vy = -13;
							
		}
	}

	//Gestion Collisions avec le blobby2 (celui a droite = IA)
	if (((blobby2.x >= ballon.x) & (blobby2.x <= (ballon.x + ballon.width))) || (((blobby2.x + blobby2.width) >= ballon.x) & 
	    ((blobby2.x + blobby2.width) <= (ballon.x + ballon.width))) || (((blobby2.x + 17.5) >= ballon.x) & ((blobby2.x + 17.5) <= (ballon.x + ballon.width))))
	{
		if (((blobby2.y <= (ballon.y + ballon.height)) & (blobby2.y >= ballon.y)) || (((blobby2.y + blobby2.height) >= ballon.y) & 
		    ((blobby2.y + blobby2.height) <= (ballon.y + ballon.height))) || (((blobby2.y + 20) >= ballon.y) & ((blobby2.y + 20) <= (ballon.y + ballon.height))))
		{
			//calcul de la nouvelle vitesse du ballon apres collision
			ballon.vx = ((ballon.x + 14)-(blobby2.x + 17.5)) / 4; 
			ballon.vy = ((ballon.y + 14)-(blobby2.y + 20)) / 4;  
						
			//Les blobbys n'on droint qu'a 3 touches consecutives
			//cette variable temps permets de ne compter qu'une touche par tranche de 25 frames	
			if (temps - last_touch_b2 > 25)
				touch_b2 ++;
				
			last_touch_b2 = temps;
			
			//blobby1 n'est pas le dernier a avoir touche la balle, on remet son compteur a 0	
			touch_b1 = 0;
			
			//Collision avec le fillet possible	
			collision_fillet = 1;
				
			//ici on gere le cas ou le ballon est a la verticla parfaite du blobby, on lui donne une legere vitesse horizontale et un vitesse verticale suffisante	
			if ((int)(((ballon.x + 14)-(blobby2.x + 17.5))) == 0.00){
				switch (rand() %2){
					case 0:
						ballon.vx = 1;
						break;
					case 1:
						ballon.vx = -1;
						break;
					}  
				ballon.vy = -3;
			}

			//limite de vitesse du ballon, sinon c'est du flipper ;)
			if (ballon.vx > 13)
				ballon.vx = 13;
			if (ballon.vx < -13)
				ballon.vx = -13;
				
			if (ballon.vy > 13)
				ballon.vy = 13;
			if (ballon.vy < -13)
				ballon.vy = -13;
		}
	}
	
	//le blobby qui touche trois fois le ballon successivement perd un point ou l'avantage
	if (touch_b1 > 3)
	{
		if(avantage == 2)
			score_b2 ++;
		avantage = 2;
		ballon.vx = 0;
		ballon.vy =0;
		ballon.x = 360;
		ballon.y = 140;
		touch_b1 = 0;
		touch_b2 = 0;
	}
	if (touch_b2 > 3)
	{
		if(avantage == 1)
			score_b1 ++;
		avantage = 1;
		ballon.vx = 0;
		ballon.vy =0;
		ballon.x = 96;
		ballon.y = 140;
		touch_b1 = 0;
		touch_b2 = 0;
	}
		
	
	
	//Gestion Collision fillet
	if (((ballon.x + ballon.width) >= 238) && (ballon.x <= 241) && (ballon.vx > 0) && ((ballon.y + ballon.height) >= 127) && (collision_fillet == 1))
	{
		//le ballon change de direction
		ballon.vx = -fabs(ballon.vx);
		// la variable collision evite 'avoir un ballon en train de bugger contre le fillet
		collision_fillet = 0;
	}
	else if (((ballon.x + ballon.width) >= 238) && (ballon.x <= 241) &&(ballon.vx < 0) && ((ballon.y + ballon.height) >= 127) && (collision_fillet == 1))
	{
		//le ballon change de direction
		ballon.vx = fabs(ballon.vx);
		// la variable collision evite 'avoir un ballon en train de bugger contre le fillet
		collision_fillet = 0;
	}
	
	//deplacement du ballon
	ballon.y = ballon.y + ballon.vy;
	ballon.x = ballon.x + ballon.vx;
	//il subit la gravite
	if ((ballon.vx != 0) | (ballon.vy != 0))
		ballon.vy = ballon.vy + gravite;
	
	//gestion collision du ballon avec le sol
	if (ballon.y + ballon.height >= niveauSol)		
	{
		//Au sol, la vitesse passe ? z?ro
		ballon.vy = 0.0f;
		ballon.vx = 0.0f;
		
		//et l'un des blobby gagne un point ou l'avantage
		if (ballon.x <= 238){
			if(avantage == 2)
				score_b2 ++;
			avantage = 2;
			touch_b1 = 0;
			touch_b2 = 0;
			ballon.x = 360;
			ballon.y = 140;
		}
		else 
		{
			if(avantage == 1)
				score_b1 ++;
			avantage = 1;
			touch_b1 = 0;
			touch_b2 = 0;
			ballon.x = 96;
			ballon.y = 140;
		}
		
		//Collision avec le filet possible
		collision_fillet = 1;
	}
		
	//gestion collision du ballon avec les extremites
	if (ballon.x <= 0)
	{
		ballon.vx = fabs(ballon.vx);	
		collision_fillet = 1;
	}
	if ((ballon.x + ballon.width) >= 480)
	{
		ballon.vx = -fabs(ballon.vx);
		collision_fillet = 1;
	}
	
}	


//Dessine le blobby
void DessineBlobby(BLOBBY &blobby, OSL_IMAGE &image)
{
	int i;
	if(blobby.auSol) 
	{
		//Choisir l'image pour dessiner le blobby p?riodiquement (1-2-3-4-3-2-1...)
		switch ((i = blobby.frameNb/8) % 6){
			case 0:
				oslSetImageFrame(&image, 1);
				oslSetImageFrame(imgOmbre, 1);
				break;
			case 1:
				oslSetImageFrame(&image, 2);
				oslSetImageFrame(imgOmbre, 2);
				break;
			case 2:
				oslSetImageFrame(&image, 3);
				oslSetImageFrame(imgOmbre, 3);
				break;
			case 3:
				oslSetImageFrame(&image, 4);
				oslSetImageFrame(imgOmbre, 4);
				break;
			case 4:
				oslSetImageFrame(&image, 3);
				oslSetImageFrame(imgOmbre, 3);
				break;
			case 5:
				oslSetImageFrame(&image, 2);
				oslSetImageFrame(imgOmbre, 2);
				break;
		}
	}
	else{
		//Sinon il est en l'air, utilise la tile du saut (0)
		oslSetImageFrame(&image, 0);
		oslSetImageFrame(imgOmbre, 0);
	}
	//Dessine l'ombre 
	oslDrawImageXY(imgOmbre, (int)(blobby.x) - 16, 223);
	//dessine le blobby
	oslDrawImageXY(&image, (int)(blobby.x), (int)(blobby.y));
}

//Dessine le ballon
void DessineBallon(BALLON &ballon)
{
	// le ballon est dessine periodiquement egalement (il tourne sur lui meme)
	oslSetImageFrame(imgBall, (ballon.frameNb/8) % 8);
	oslDrawImageXY(imgOmbreBall, (int)(ballon.x), 230);
	oslDrawImageXY(imgBall, (int)(ballon.x), (int)(ballon.y));
}

//Retourne un objet blobby initialis?
BLOBBY CreeBlobby(int x){
	BLOBBY blobby;
	//image 0 a utilisee en premier pour dessiner le blobby
	blobby.frameNb = 0;
	//Taille du perso = taille d'une image d'un blobby
	blobby.width = 35;
	blobby.height = 40;
	//Position initiale
	blobby.x = x;
	blobby.y = niveauSol - blobby.height + 50;
	//Vitesses initiales nulles
	blobby.vy = 0.0f;
	//blobby au sol
	blobby.auSol = true;
	return blobby;
}

//Retourne un objet ballon initialise
BALLON CreeBallon() {
	BALLON ballon;
	//image 0 a utiliser en premier pour dessiner le ballon
	ballon.frameNb = 0;
	//Taille du ballon calcul?e via l'image
	ballon.width = 28;
	ballon.height = 28;
	//Position initiale
	ballon.x = 96.0f;
	ballon.y = 150.0f;
	//Vitesses initiales nulles
	ballon.vx = 0.0f;
	ballon.vy = 0.0f;
	return ballon;
}


//Fonction qui test le score des joueurs pour savoir si il y a un gagnant (21 points + 2 points d'ecarts)
int TestScore (int score_joueur1, int score_joueur2)
{
	if((score_joueur1 >= 21) && (score_joueur1 > score_joueur2 + 1))
		return 1;
	if((score_joueur2 >= 21) && (score_joueur2 > score_joueur1 + 1))
		return 2;
	return 0;
}

//Ecrire avec les caracteres des images imgFontMaj et imgFontMin a partir d'un entier < 99
void Ecrire(const int nombre, int x, int y)
{

	int unite = nombre;
	int dizaine = 0;
	
	//on separe les dizaines des unites
	while(unite > 9)
	{
		unite -= 10;
		dizaine ++;
	}
	//et on affiche
	oslSetImageFrame(imgFontMaj, dizaine);
	oslDrawImageXY(imgFontMaj, x, y);
	oslSetImageFrame(imgFontMaj,unite%10);
	oslDrawImageXY(imgFontMaj, x + 24, y);
}


//Ecrire avec les caracteres des images imgFontMaj et imgFontMin a partir d'une chaine de caracteres
void Ecrire(const char *phrase, int x, int y)
{
	unsigned int i;
	char a;
	int tempx = x;
	
	//***fonction non complete***: je n'ai pas inclus tous les caracteres seulement ceux dont j'avais besoin
	//la variable tempx sert a savoir de combien on se decale pour ecrire la prochaine lettre selon si on a ecrit une minuscule ou une majuscule
	for( i = 0; i <= strlen(phrase); i++){
		a = phrase[i];
		//0-9
		if( a >= 48 && a <= 57){
			oslSetImageFrame(imgFontMaj, a - 48);
			oslDrawImageXY(imgFontMaj, tempx, y);
			tempx = tempx + 24;
		}
		//minuscule
		if (a >= 97 && a <= 122){
			oslSetImageFrame(imgFontMin, a - 87);
			oslDrawImageXY(imgFontMin, tempx, y + 10);
			tempx = tempx + 14;
		}
		//espace
		if (a == 32){
			oslSetImageFrame(imgFontMin, 51);
			oslDrawImageXY(imgFontMin, tempx, y + 10);
			tempx = tempx + 14;
		}
		//majuscule
		if (a >= 65 && a <= 90) {
			oslSetImageFrame(imgFontMaj, a - 55);
			oslDrawImageXY(imgFontMaj, tempx, y);
			tempx = tempx + 24;
		}
		//!
		if (a == 33){
			oslSetImageFrame(imgFontMaj, 37);
			oslDrawImageXY(imgFontMaj, tempx, y);
			tempx = tempx + 24;
		}
		//-
		if (a == 45){
			oslSetImageFrame(imgFontMin, 50);
			oslDrawImageXY(imgFontMin, tempx, y + 10);
			tempx = tempx + 14;
		}
		// si fin de la chaine ('\0') alors on s'arrete
		if (a == '\0')
			return;
		
	}
	
}

//la fonction main qui est en fait le menu
int main()
{

	//la couleur de fond des images de blobbys, balon... = couleur transparente
	const OSL_COLOR couleurMasque = RGB(0, 0, 0);

	bool skip = false;
	//focus pour le menu	
	int focus = 0;
	
	//Initialisation de OSLib
	oslInit(0);
	oslInitGfx(OSL_PF_5650, 1);
	oslSetQuitOnLoadFailure(1);
	
	// la il faudrait verifier que les images sont bien presentes sinon plantage... :(

	//Chargement des images
	imgFond = oslLoadImageFilePNG("img/imgFond.png", OSL_IN_VRAM, OSL_PF_5650);
		
	//les images suivantes ont une couleur transparente
	oslSetTransparentColor(couleurMasque);
	
		imgBlobbyR = oslLoadImageFilePNG("img/imgBlobbyR.png", OSL_IN_RAM, OSL_PF_5551);
		imgBlobbyB = oslLoadImageFilePNG("img/imgBlobbyB.png", OSL_IN_RAM, OSL_PF_5551);
		imgOmbre = oslLoadImageFilePNG("img/imgOmbre.png", OSL_IN_RAM, OSL_PF_5551);
		imgBall = oslLoadImageFilePNG("img/imgBall.png", OSL_IN_RAM, OSL_PF_5551);
		imgOmbreBall = oslLoadImageFilePNG("img/imgOmbreBall.png", OSL_IN_RAM, OSL_PF_5551);
		imgTitre = oslLoadImageFilePNG("img/imgTitre.png", OSL_IN_RAM, OSL_PF_5551);
		imgFontMin = oslLoadImageFilePNG("img/imgFontMin.png", OSL_IN_RAM, OSL_PF_5551);
		imgFontMaj = oslLoadImageFilePNG("img/imgFontMaj.png", OSL_IN_RAM, OSL_PF_5551);
		imgTrophee = oslLoadImageFilePNG("img/imgTrophee.png", OSL_IN_RAM, OSL_PF_5551);
	
	oslDisableTransparentColor();
	
	//images divis?es en plusieurs mosa?ques (blobby, ballon...)
	oslSetImageFrameSize(imgBlobbyR, 35, 40);
	oslSetImageFrameSize(imgBlobbyB, 35, 40);
	oslSetImageFrameSize(imgOmbre, 77, 19);
	oslSetImageFrameSize(imgBall, 28, 28);
	oslSetImageFrameSize(imgFontMin, 14, 14);
	oslSetImageFrameSize(imgFontMaj, 24, 24);
	
	while (!osl_quit){
		
		//Doit on faire le rendu?
		if (!skip) 
		{	
			oslStartDrawing();
			//on initialise l'ecran en noir
			oslClearScreen(RGB(0, 0, 0));
			
			//active la tranparence - donne un effet un peu terne grace au fond noir
			oslSetAlpha(OSL_FX_ALPHA,64);
			
				//on dessine notre fond (terne pour ceux qui suivent :)
				oslDrawImage(imgFond);

			//desactive la transparence
			oslSetAlpha(OSL_FX_RGBA,64);
			
			//le titre
			oslDrawImage(imgTitre);
			
			//menu avec choix en surbrillance
			switch (focus)
			{
				case 0:
					Ecrire("Play VS IA", 190, 160);
					
					oslSetAlpha(OSL_FX_ALPHA,64);
						Ecrire("Quit", 190, 190); //terne
					oslSetAlpha(OSL_FX_RGBA,64);
					break;
				case 1:
					Ecrire("Quit", 190, 190);
					
					oslSetAlpha(OSL_FX_ALPHA,64);
						Ecrire("Play VS IA", 190, 160); //terne			
					oslSetAlpha(OSL_FX_RGBA,64);
					break;
			}
						
			oslEndFrame();
			oslEndDrawing();
		}
		
		//Gestion des touches
		oslReadKeys();
		
		if (osl_pad.pressed.down)
		{
			focus ++;
			if (focus > 0)
				focus = 1;
		}
		
		if (osl_pad.pressed.up)
		{
			focus --;
			if (focus < 0)
				focus = 0;
		}
		
		
		if (osl_pad.pressed.cross)	
		{
			// On lance le jeu
			if (focus == 0)
			{
				game_main(focus);
				
			}
			//Bye bye
			if (focus == 1){
				oslEndGfx(); 
				oslQuit(); 
				return 0;
			}
		}
		
		//Synchro ? 60 fps
		skip = oslSyncFrameEx(0, 6, 0);
					
	}
		
	oslEndGfx(); 
	oslQuit(); 
	return 0;
}



//Boucle Principale (le jeu)
int game_main(int option) 
{ 
	bool pause = false;
	bool skip = false;
	
	BLOBBY Blobby1;
	BLOBBY Blobby2;
	BALLON Ballon1;
	
	//Pour que les touches se repetent lorqu'on les gardes enfoncees
	oslSetKeyAutorepeat(OSL_KEYMASK_RIGHT|OSL_KEYMASK_LEFT|OSL_KEYMASK_CROSS, 1, 1);
	
	//On cree les blobbys et le ballon
	Blobby1 = CreeBlobby(96);
	Blobby2 = CreeBlobby(360);
	Ballon1 = CreeBallon();
        
	//Initialisations de d?but de partie
	avantage = 1;
	score_b1 = 0;
	score_b2 = 0;

	
	//initialisation de notre variables temps en rapport avec le compatge des trois touches consecutives du ballon (voir fonction GereBallon)
	temps = 0.0f;

	
	while (!osl_quit)		
	{
		
		//Doit on faire le rendu? - synchro a 60FPS			
		if (!skip) 
		{	
			oslStartDrawing();
		
			oslClearScreen(RGB(0, 0, 0));
	
			//Fin du jeu?
			if (TestScore(score_b1,score_b2) == 1)
			{
				
				//active la tranparence
				oslSetAlpha(OSL_FX_ALPHA,64);

					oslDrawImage(imgFond);

				//desactive la transparence
				oslSetAlpha(OSL_FX_RGBA,64);

				Ecrire("Winner!", 190, 10);
				oslDrawImageXY(imgTrophee, 200, 60);
				Ecrire("Press start to go to the menu", 20, 240);
				//si on appuie sur start on retourne au menu				
				if(osl_pad.pressed.start)
					return 1;
				
			}
			//fin du jeu?
			else if (TestScore(score_b1,score_b2) == 2)
			{
				
				//active la tranparence
				oslSetAlpha(OSL_FX_ALPHA,64);

				oslDrawImage(imgFond);

				//desactive la transparence
				oslSetAlpha(OSL_FX_RGBA,64);				
				
				Ecrire("Loser!", 190, 10);
				oslDrawImageXY(imgTrophee, 210, 60);
				Ecrire("Press start to go to the menu", 20, 240);
				//si on appuie sur start on retourne au menu
				if(osl_pad.pressed.start)
					return 1;
				
			}
			//pause?
			else if (pause == true)
			{
								
				//active la tranparence
				oslSetAlpha(OSL_FX_ALPHA,64);

				oslDrawImage(imgFond);

				//desactive la transparence
				oslSetAlpha(OSL_FX_RGBA,64);
			
				Ecrire("PAUSE", 185, 100);
				Ecrire("Press Start to resume", 85, 130);
				
			}
			//sinon on joue ;)
			else if (temps > 30)
			{
				//Dessin du fond
				oslDrawImage(imgFond);
				
				//On dessine les blobbys et le ballon
				DessineBallon(Ballon1);
				DessineBlobby(Blobby1, *imgBlobbyR);
				DessineBlobby(Blobby2, *imgBlobbyB);
				
				GereBlobby1(Blobby1);
				GereBlobbyIA(Blobby2, Ballon1);
				GereBallon(Ballon1, Blobby1, Blobby2);
	
			}
		
		//Affiche le score
		Ecrire(score_b1, 10, 5);
		Ecrire(score_b2, 420, 5);
		
		//! pour l'avantage
		if(avantage == 1)
			Ecrire("!", 54, 5);
		else
			Ecrire("!", 394, 5);
		
		oslEndFrame();
		oslEndDrawing();
			
		}
		
	//Pour pouvoir g?rer les touches
	oslReadKeys();
	
	//Gestion du menu pause
	if (osl_pad.pressed.start)	{
		pause = !pause;
	}
	
	//on incremente la variable temps (en rapport avec le compatge des trois touches consecutives du ballon (voir fonction GereBallon))
	temps = temps + 1.0f;

	//Synchro ? 60 fps
	skip = oslSyncFrameEx(0, 6, 0);
	
	}

	oslEndGfx(); 
	oslQuit(); 
	return 0; 
} 

