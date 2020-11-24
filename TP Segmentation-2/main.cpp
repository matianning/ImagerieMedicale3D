/* 
	Compilation:
	(Linux  version) g++  -o main.exe main.cpp -O2 -L/usr/X11R6/lib  -lm  -lpthread -lX11

	Execution :
	main.exe liver_07.liver.norm.hdr
*/
	

#include "CImg.h"
#include<iostream>
#include <vector>
#include <algorithm>
#include <set>
using namespace cimg_library;

float seuil = 200.0;
int nb_fois_convolution = 3;
int maxIter = 10;
int pos_x(0), pos_y(0), pos_z(0);
unsigned char color[] = {255,255,255};
int tolerance = 0;
bool seuiller = false;
struct Point{
	int x;
	int y;
	int z;
};
typedef struct Point point;

bool operator==(const Point& left, const Point& right)
{
    return ((left.x==right.x) && (left.y==right.y));
}

bool operator<(const Point& left, const Point& right)
{
    return ((left.x<=right.x) && (left.y<=right.y));
}

bool operator>(const Point& left, const Point& right)
{
    return ((left.x>right.x) && (left.y>right.y));
}


std::vector<point> graines;	//graines à itérer pour la croissance par région
std::vector<point> res;		//point à déssiner sur l'image
std::vector<point> marque;	//marqueur si un pixel est déjà traité

/* Main program */
int main(int argc,char **argv)
{
	/* Create and load the 3D image */
	CImg<> img;
	float voxelsize[3];
	/* Load in Analyze format and get the voxel size in an array */
	img.load_analyze(argv[1],voxelsize);
	

	/* Get the image dimensions */
    int dim[]={img.width(),img.height(),img.depth()}; 
	printf("Reading %s. Dimensions=%d %d %d\n",argv[1],dim[0],dim[1],dim[2]);
	printf("Voxel size=%f %f %f\n",voxelsize[0],voxelsize[1],voxelsize[2]);
		
	/* Create the display window of size 512x512 */
	CImgDisplay disp(512,512,"Segmentation 2");    
	
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
	
	/* Manage the display windows: ESC, or closed -> close the main window */
	while (!disp.is_closed() && !disp.is_keyESC()) // Main loop
	{
		/* List of events */
		/***** Intéraction des touches *****/
		if (disp.is_keyM()){ img.blur(2.0,2.0,2.0,true,false); redraw = true; }
		if (disp.is_keyN()){ img.sharpen(20.0,20.0,20.0,true,false); redraw = true;}

		if(disp.is_keyPADADD()){ seuil+=5; redraw = true; }
		if(disp.is_keyPADSUB()){seuil-=5; redraw = true; }
		if(disp.is_keyPADMUL()){ nb_fois_convolution++; redraw = true; }
		if(disp.is_keyPADDIV()){ nb_fois_convolution--; redraw = true;}

		if(disp.is_keyDELETE()){}

		/* Resizing */
		if (disp.is_resized()) 
		{
			disp.resize();
		}
		/* Movement of the mouse */
		
		/* If the mouse is inside the display window, find the active quadrant 
		and the relative position within the active quadrant */
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
		
		/* Click Right button to get a position */
		if (disp.button()&2  && (plane!=-1))  
		{
			for(unsigned int i=0;i<3;i++) 
			{
				displayedSlice[i]=coord[i];
			}
			redraw = true;
		}

		/* Wheel interaction */
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

		seuiller = false;

		/* Left Click pour choisir la position de graine*/
		if (disp.button()&1  && (plane!=-1))  
		{
			
			pos_x = disp.mouse_x();
			pos_y = disp.mouse_y();
			pos_z = displayedSlice[plane];
			//std::cout<<"Graine choisie : ["<<pos_x<<","<<pos_y<<","<<pos_z<<"]"<<std::endl;

			redraw = true;
			seuiller = true;
		}


		
		if (redraw)
		{
			
			CImg<> mpr_img=img.get_projections2d(displayedSlice[0],displayedSlice[1],displayedSlice[2]);  
			mpr_img.resize(512,512); 
			


			//******* Prétraitement *******
			
			//******* Filtre médian --- Réduction bruit ******
			mpr_img = mpr_img.get_blur_median(3.0);

			//******* - Opération - Erosion *******
			/*
			mpr_img = mpr_img.erode(nb_fois_convolution,nb_fois_convolution,nb_fois_convolution);
			mpr_img = mpr_img.dilate(nb_fois_convolution,nb_fois_convolution,nb_fois_convolution);
			*/

			//******** Filtre laplacien --- compensation du fond ********
			CImg<> filtre_laplacien = CImg<>::matrix(0,-1,0,-1,4,-1,0,-1,0);
			//mpr_img.convolve(filtre_laplacien);
			CImg<> result = mpr_img;
			
			if(pos_x!=0 || pos_y!=0){
				point graine;
				graine.x = pos_x; graine.y = pos_y; graine.z = pos_z;
				graines.clear(); graines.push_back(graine);

				int compteur = 0;
				//***Croissance par région***
				for(int i = 0; i < maxIter; i++){
					int current_size = graines.size();
					for(int j = compteur; j < current_size; j++){
						compteur++;
						int current_x = graines[j].x;
						int current_y = graines[j].y;
						int current_z = graines[j].z;
						int current_value = mpr_img(current_x, current_y);	

						if(current_x - 1 >= 0){
							if(mpr_img(current_x - 1, current_y) >= current_value -tolerance){
								point newGraine; newGraine.x = current_x - 1; newGraine.y = current_y; newGraine.z = current_z;
								graines.push_back(newGraine);
							}
						}
						if(mpr_img(current_x + 1, current_y) >= current_value-tolerance){
							point newGraine; newGraine.x = current_x + 1; newGraine.y = current_y; newGraine.z = current_z;
							graines.push_back(newGraine);
						}
						if(current_y - 1 >= 0){
							if(mpr_img(current_x, current_y - 1) >= current_value -tolerance){
								point newGraine; newGraine.x = current_x; newGraine.y = current_y - 1; newGraine.z = current_z;
								graines.push_back(newGraine);
							}
						}
						if(mpr_img(current_x, current_y + 1) >= current_value-tolerance){
							point newGraine; newGraine.x = current_x; newGraine.y = current_y + 1; newGraine.z = current_z;
							graines.push_back(newGraine);
						}

					}
					
					//std::sort(graines.begin(), graines.end());
					//graines.erase(std::unique(graines.begin(), graines.end()), graines.end());
					
				}
			}
			
			//std::cout<<"graines size : "<<graines.size()<<std::endl;
			
			//*****dessiner dans l'image de sortie*******
			for(int i = 0; i < graines.size(); i++){
				result.draw_circle(graines[i].x, graines[i].y, 1, color);
			}

			//*****Seuiller l'image de sortie********
			if(seuiller){
				result = result.get_threshold(seuil);
				CImg<> labels = result.get_label(false);
				for(size_t x = 0; x < labels.width(); x++){
					for(size_t y = 0; y < labels.height(); y++){
						for(size_t z = 0; z < labels.depth(); z++){
						 	if(labels.atXYZ(x,y,z) == labels.atXYZ(pos_x, pos_y, pos_z)){	//même label que la graine
						 		result.draw_circle(x, y, 1, color);
						 	}
						}
					}
				}
			}
		
			disp.display(result.abs().normalize(0,255));
	
			redraw=false;
		}


	}


	return 0;
}


