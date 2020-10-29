/* 	HMIN318
	Compilation:
	(Linux  version) g++  -o main.exe main.cpp -O2 -L/usr/X11R6/lib  -lm  -lpthread -lX11

	Execution :
	./main.exe 
*/
	
/* Warning! the MPR value does not deal with the voxel size. It is assumed isotropic. */ 	

#include "CImg.h"
#include <iostream>
#include <limits>
#include <string>
#include <vector>
#include <stdio.h>
#include <stdlib.h>

using namespace cimg_library;

float seuil = 22.0f;
float n_ero = 3.0;



void MIP(CImg<float>& img_in, CImg<float>& img_out){

	for(int y = 0 ; y < img_in.height() ; y++)
	{
		for(int x = 0 ; x < img_in.width() ; x++)
		{
			float current_max = std::numeric_limits<float>::min();
			int z_max = 0;

			for(int z = 0 ; z < img_in.depth() ; z++)
			{
				if(img_in(x, y, z) > current_max) {
					current_max = img_in(x, y, z);
					z_max = z;
				}
			}

			img_out(x, y, z_max) = current_max;
		}
	}
}

float distance (float x1, float y1, float z1, float x2, float y2, float z2){
	return sqrt(pow((x2-x1),2)+pow((y2-y1),2)+pow((z2-z1),2));
}

/* Main program */
int main(int argc,char **argv)
{
	/*
	if(argc != 2)
	{
		printf("Usage : %s filename.hdr\n", argv[0]);
		exit(EXIT_FAILURE);
	}
*/
	/* Create and load the 3D image */
	CImg<float> img, img1, img2, img3;
	float voxelsize[3];
	/* Load in Analyze format and get the voxel size in an array */
	img.load_analyze("DATA1/stack-0.hdr",voxelsize);
	img1.load_analyze("DATA1/stack-1.hdr",voxelsize);
	img2.load_analyze("DATA1/stack-2.hdr",voxelsize);
	img3.load_analyze("DATA1/stack-3.hdr",voxelsize);

	/* Get the image dimensions */
    int dim[]={img.width(),img.height(),img.depth()}; 
	printf("Reading %s. Dimensions=%d %d %d\n",argv[1],dim[0],dim[1],dim[2]);
	printf("Voxel size=%f %f %f\n",voxelsize[0],voxelsize[1],voxelsize[2]);


	unsigned int voxel_coord[] = {256, 256, 12};
	printf("Voxel(%u,%u,%u) = %lf\n", voxel_coord[0], voxel_coord[1], voxel_coord[2], img(voxel_coord[0], voxel_coord[1], voxel_coord[2]));

	/* Create the display window of size 512x512 */
	CImgDisplay disp(512*4,512,"");
	printf("disp.normalization() = %d\n", disp.normalization());
	
	/* The 3 displayed slices of the MPR visualisation */
	int displayedSlice[]={dim[0]/2,dim[1]/2,dim[2]/2}; 
		
	/* Slice corresponding to mouse position: */
	unsigned int coord[]={0,0,0};
	/* The display window corresponds to a MPR view which is decomposed into the following 4 quadrants: 
	2=original slice size=x y        0 size=z y
	1= size=x z                     -1 corresponds to the 4th quarter where there is nothing displayed */
	int plane=2;
	
	/* For a first drawing, activate the redrawing flag */
	bool redraw=true;
	std::string visu_mode = "MPR";
	/* Manage the display windows: ESC, or closed -> close the main window */
	while (!disp.is_closed() && !disp.is_keyESC()) // Main loop
	{
		/* List of events */
		
		/* Resizing */
		if (disp.is_resized()) 
		{
			disp.resize();
		}

		if (disp.is_key("m"))
		{
			img.blur(0.5);
		}

		if (disp.is_key("r"))
		{
			visu_mode = "MPR";
		}

		if (disp.is_key("a"))
		{
			visu_mode = "MIP";
		}

		if (disp.is_key("z"))
		{
			visu_mode = "MinIP";
		}

		if (disp.is_key("e"))
		{
			visu_mode = "AIP";
		}


		if(disp.mouse_x()>=0 && disp.mouse_y()>=0) 
		{
			unsigned int mX = disp.mouse_x()*(dim[0]+dim[2])/disp.width();
			unsigned int mY = disp.mouse_y()*(dim[1]+dim[2])/disp.height();
			if (mX>=dim[0] && mY<dim[1]) 
			{ 
				plane = 0; 
				coord[1] = mY; 
				coord[2] = mX - dim[0];   
				coord[0] = displayedSlice[0]; 
			}
			else 
			{
				if (mX<dim[0] && mY>=dim[1]) 
				{ 
					plane = 1; 
					coord[0] = mX; 
					coord[2] = mY - dim[1];   
					coord[1] = displayedSlice[1]; 
				}
				else 
				{
					if (mX<dim[0] && mY<dim[1])       
					{ 
						plane = 2; 
						coord[0] = mX; 
						coord[1] = mY;     
						coord[2] = displayedSlice[2]; 
					}
					else 
					{
						plane = -1; 
						coord[0] = 0;
						coord[1] = 0;
						coord[2] = 0;
					}
				}
			}
			redraw = true;
		}
		
		if (disp.button()&2  && (plane!=-1))  
		{
			for(unsigned int i=0;i<3;i++) 
			{
				displayedSlice[i]=coord[i];
			}
			redraw = true;
		}

		if (disp.wheel()) 
		{
			displayedSlice[plane]=displayedSlice[plane]+disp.wheel();
			
			if (displayedSlice[plane]<0) 
			{
				displayedSlice[plane] = 0;
			}
			else 
			{
				if (displayedSlice[plane]>=(int)dim[plane])
				{
					displayedSlice[plane] = (int)dim[plane]-1;
				}
			}
			
		/* Flush all mouse wheel events in order to not repeat the wheel event */
		disp.set_wheel();
		redraw = true;
		}
		




		if (redraw)
		{

			CImg<float> visu = img;
			CImg<float> mpr_img = visu.get_projections2d(displayedSlice[0],displayedSlice[1],displayedSlice[2]);
			
			//2.1 -Utiliser un filtre(par exemple médian)pour éliminer le bruit dans les données

			CImg<> res = img.get_blur_median(3.0);
			CImg<> res1 = img1.get_blur_median(3.0);
			CImg<> res2 = img2.get_blur_median(3.0);
			CImg<> res3 = img3.get_blur_median(3.0);

			//2.2 -Utiliser un seuil pouréliminer le fond et de ne garder que les noyaux cellulaires.

			res = res.get_threshold(seuil);
			res1 = res1.get_threshold(seuil);
			res2 = res2.get_threshold(seuil);
			res3 = res3.get_threshold(seuil);

			//2.3 -Éliminerles quelques pixels isolés par morphologie mathématique
			res.erode(n_ero,n_ero,n_ero); res.dilate(n_ero,n_ero, n_ero);
			res1.erode(n_ero,n_ero,n_ero); res1.dilate(n_ero,n_ero, n_ero);
			res2.erode(n_ero,n_ero,n_ero); res2.dilate(n_ero,n_ero, n_ero);
			res3.erode(n_ero,n_ero,n_ero); res3.dilate(n_ero,n_ero, n_ero);

			//2.4 -Identifierles cellulespar composantes connexes
			CImg<> labels = res.get_label(false);
			CImg<> labels1 = res1.get_label(false);
			CImg<> labels2 = res2.get_label(false);
			CImg<> labels3 = res3.get_label(false);

			//2.5 -Calculer le barycentre de chacune des cellules et le sauvegarder dans un fichier
			//compteur pour chaque labels de l'image
			std::vector<int> compteur(labels.max() + 1 , 0 ); 
			std::vector<int> compteur1(labels1.max() + 1 , 0 ); 
			std::vector<int> compteur2(labels2.max() + 1 , 0 ); 
			std::vector<int> compteur3(labels3.max() + 1 , 0 ); 
			std::vector<float> coord = {0.0f,0.0f,0.0f};
			//somme[i][0] : x somme[i][1] : y somme[i][2] : z
			std::vector<std::vector<float>> somme(compteur.size(),coord); 
			std::vector<std::vector<float>> somme1(compteur1.size(),coord); 
			std::vector<std::vector<float>> somme2(compteur2.size(),coord); 
			std::vector<std::vector<float>> somme3(compteur3.size(),coord); 

			for(size_t x = 0; x < labels.width(); x++){
				for(size_t y = 0; y < labels.height(); y++){
					for(size_t z = 0; z < labels.depth(); z++){
						 if (labels.atXYZ(x, y, z) > 0) { 	// Enlever label du fond 
						 	//Pour image 0
							compteur[(int)labels.atXYZ(x,y,z)]++;
							somme[(int)labels.atXYZ(x,y,z)][0] += x;
							somme[(int)labels.atXYZ(x,y,z)][1] += y;
							somme[(int)labels.atXYZ(x,y,z)][2] += z;
							//Pour image 1
							compteur1[(int)labels1.atXYZ(x,y,z)]++;
							somme1[(int)labels1.atXYZ(x,y,z)][0] += x;
							somme1[(int)labels1.atXYZ(x,y,z)][1] += y;
							somme1[(int)labels1.atXYZ(x,y,z)][2] += z;
							//Pour image 2
							compteur2[(int)labels2.atXYZ(x,y,z)]++;
							somme2[(int)labels2.atXYZ(x,y,z)][0] += x;
							somme2[(int)labels2.atXYZ(x,y,z)][1] += y;
							somme2[(int)labels2.atXYZ(x,y,z)][2] += z;
							//Pour image 3
							compteur3[(int)labels3.atXYZ(x,y,z)]++;
							somme3[(int)labels3.atXYZ(x,y,z)][0] += x;
							somme3[(int)labels3.atXYZ(x,y,z)][1] += y;
							somme3[(int)labels3.atXYZ(x,y,z)][2] += z;
						}
					}
				}
			}

			//Écriture de fichiers pour sauvegarder les barycentres
			//ici j'ai crée les vectors pour sauvegarder ces données aussi juste pour faciliter la tache
			std::vector<std::vector<float>> barycentres(compteur.size(),coord); 
			std::vector<std::vector<float>> barycentres1(compteur1.size(),coord); 
			std::vector<std::vector<float>> barycentres2(compteur2.size(),coord); 
			std::vector<std::vector<float>> barycentres3(compteur3.size(),coord); 
			FILE* fichier = NULL;
		 	int nb_fichier = 4;
		 	std::cout << "Ecriture des fichiers en cours..." << std::endl;
		 	for(int f = 0; f < nb_fichier; f++){
		 		 std::string filename = "stack";
		 		 char buffer[sizeof(int)]; sprintf(buffer,"%d", f);
		 		 filename += buffer;
		 		 filename +=".txt"; 
		 		 std::cout<<filename; 
		 		 fichier = fopen(filename.c_str(),"w+");
		 
			    if (fichier != NULL)
			    {
			    	if(f == 0){
			    		for(int i = 0; i < somme.size(); i++){
						fprintf(fichier, "%d : %f, %f, %f \n", i, 
							somme[i][0] / (float) compteur[i], somme[i][1] / (float) compteur[i], somme[i][2] / (float) compteur[i]);
						barycentres[i][0] = somme[i][0] / (float) compteur[i];
						barycentres[i][1] = somme[i][1] / (float) compteur[i];
						barycentres[i][2] = somme[i][2] / (float) compteur[i];
						}
			       		fclose(fichier);
			    	}
			    	if(f == 1){
			    		for(int i = 0; i < somme1.size(); i++){
						fprintf(fichier, "%d : %f, %f, %f \n", i, 
							somme1[i][0] / (float) compteur1[i], somme1[i][1] / (float) compteur1[i], somme1[i][2] / (float) compteur1[i]);
						barycentres1[i][0] = somme1[i][0] / (float) compteur1[i];
						barycentres1[i][1] = somme1[i][1] / (float) compteur1[i];
						barycentres1[i][2] = somme1[i][2] / (float) compteur1[i];
						}	
			       		fclose(fichier);
			    	}
			    	if(f == 2){
			    		for(int i = 0; i < somme2.size(); i++){
						fprintf(fichier, "%d : %f, %f, %f \n", i, 
							somme2[i][0] / (float) compteur2[i], somme2[i][1] / (float) compteur2[i], somme2[i][2] / (float) compteur2[i]);
						barycentres2[i][0] = somme2[i][0] / (float) compteur2[i];
						barycentres2[i][1] = somme2[i][1] / (float) compteur2[i];
						barycentres2[i][2] = somme2[i][2] / (float) compteur2[i];
						}
			       		fclose(fichier);
			    	}
			    	if(f == 3){
			    		for(int i = 0; i < somme3.size(); i++){
						fprintf(fichier, "%d : %f, %f, %f \n", i, 
							somme3[i][0] / (float) compteur3[i], somme3[i][1] / (float) compteur3[i], somme3[i][2] / (float) compteur3[i]);
						barycentres3[i][0] = somme3[i][0] / (float) compteur3[i];
						barycentres3[i][1] = somme3[i][1] / (float) compteur3[i];
						barycentres3[i][2] = somme3[i][2] / (float) compteur3[i];
						}
			       		fclose(fichier);
			    	}
			    }
		 	}
		   
			disp.display((res,res1,res2,res3));

			//3. Algorithme de suivi de cellule
			//On a <barycentre> qui corresponds à l'ensemble de coordonnée des cellules dans une image
			int max_cellule = compteur.size(); if(max_cellule < compteur1.size()) max_cellule = compteur1.size();
			if(max_cellule < compteur2.size()) max_cellule = compteur2.size();
			if(max_cellule < compteur3.size()) max_cellule = compteur3.size();

			std::vector<int> tmp(nb_fichier, -1);
			std::vector<std::vector<int>> trajectoire(max_cellule, tmp);
			for(int i = 0; i < compteur.size(); i++){trajectoire[i][0] = i;}
			std::cout<<std::endl;
			std::cout<<"Algorithme de suivi de cellule en cours..."<<std::endl;
			
			for(int i = 0; i < nb_fichier; i++){
				if(i!=nb_fichier - 1) { //sauf pour la dernière image

					for(int j = 0; j < compteur.size(); j++){ // pour chaque cellule dans l'image de départ
						
						if(i == 0){
							float min_dist = 10000.0f;
							for(int k = 0; k < compteur1.size(); k++){	//pour chaque cellule dans l'image 1
								float current_dist = distance(barycentres[j][0], barycentres[j][1], barycentres[j][2],
															  barycentres1[k][0], barycentres1[k][1], barycentres1[k][2]);
								if(min_dist > current_dist){
									min_dist = current_dist;
									trajectoire[j][i+1] = k; 
								}
							}
						}
						if(i == 1){
							float min_dist = 10000.0f;
							for(int k = 0; k < compteur2.size(); k++){
								float current_dist = distance(barycentres[j][0], barycentres[j][1], barycentres[j][2],
															  barycentres2[k][0], barycentres2[k][1], barycentres2[k][2]);
								if(min_dist > current_dist){
									min_dist = current_dist;
									trajectoire[j][i+1] = k; 
								}
							}
						}

						if(i == 2){
							float min_dist = 10000.0f;
							for(int k = 0; k < compteur3.size(); k++){
								float current_dist = distance(barycentres[j][0], barycentres[j][1], barycentres[j][2],
															  barycentres3[k][0], barycentres3[k][1], barycentres3[k][2]);
								if(min_dist > current_dist){
									min_dist = current_dist;
									trajectoire[j][i+1] = k; 
								}
							}
						}
					}
				}
			}

/*
			for(int i = 0; i < compteur.size(); i++){
				std::cout<<trajectoire[i][0]<<"-"<<trajectoire[i][1]<<"-"<<trajectoire[i][2]<<"-"<<trajectoire[i][3]<<std::endl;
			}
*/
			//Ecriture de fichier obj
			std::cout << "Ecriture de fichier OBJ..."<<std::endl;
			FILE* fichierObj = NULL;
		    int counter = 1;
		 
		    fichierObj = fopen("Trajectoire.obj", "w+");
		 

		    if (fichier != NULL)
		    {
		    	for(int i = 0; i < trajectoire.size(); i++){
		    		if(trajectoire[i][0]==-1 || trajectoire[i][1]==-1 || trajectoire[i][2]==-1 || trajectoire[i][3]==-1 ) continue;
		    		fprintf(fichierObj, "v %f %f %f\nv %f %f %f\nv %f %f %f\nv %f %f %f\nl %d %d %d %d \n", 
		    			barycentres[trajectoire[i][0]][0],barycentres[trajectoire[i][0]][1],barycentres[trajectoire[i][0]][2],
		    			barycentres1[trajectoire[i][1]][0],barycentres1[trajectoire[i][1]][1],barycentres1[trajectoire[i][1]][2],
		    			barycentres2[trajectoire[i][2]][0],barycentres2[trajectoire[i][2]][1],barycentres2[trajectoire[i][2]][2],
		    			barycentres3[trajectoire[i][3]][0],barycentres3[trajectoire[i][3]][1],barycentres3[trajectoire[i][3]][2],
		    			counter, counter+1, counter+2, counter+3);
		    	counter+=4;
		    	}
		        
		        fclose(fichier);
		    }
 














			redraw=false;
		}
	}
	return 0;
}


