
#include "fileio.hpp"
#include "uquadtree.hpp"
#include <math.h>

using namespace std;
using mapnik::Envelope;

void ply_header(FILE *fp,int num_tris,int num_verts,bool ascii,bool color,bool conf){
  fseek(fp, 0, SEEK_SET); 
  fprintf(fp,"ply\n");
  if(ascii)
    fprintf(fp,"format ascii 1.0\n");
  else
    fprintf(fp,"format binary_little_endian 1.0\n");
  fprintf(fp,"element vertex %012d\n",num_verts);
  fprintf(fp,"property float x\n");
  fprintf(fp,"property float y\n");
  fprintf(fp,"property float z\n");
  if(color){
    fprintf(fp,"property float diffuse_red\n");
    fprintf(fp,"property float diffuse_green\n");
    fprintf(fp,"property float diffuse_blue\n");
  }
  if(conf)
    fprintf(fp,"property float confidence\n");
  
    
  fprintf(fp,"element face %012d\n",num_tris);
  fprintf(fp,"property list uchar int vertex_indices\n");
  fprintf(fp,"end_header\n");
}


/**
 * Loads from a netCDF file.
 * Elevation values are assumed to be integer meters.  Projection is
 * assumed to be geographic.
 *
 * You should call SetupConversion() after loading if you will be doing
 * heightfield operations on this grid.
 *
 * \returns \c true if the file was successfully opened and read.
 */
void load_hm_file(HeightMapInfo *hm,const char *filename){
  FILE *fp= fopen(filename,"rb");
  if(!fp){
    fprintf(stderr,"Cannot open %s\n",filename);
    return;
  }
  float data[2];
  int idata[2];
	
  fread((char *)data,sizeof(float),2,fp);
  hm->x_origin=data[0];
  hm->y_origin=data[1];

  fread((char *)data,sizeof(float),2,fp);
  float min=data[0];
  float max=data[1];
  float zrange = max-min;
  fread((char *)idata,sizeof(int),2,fp);
  hm->YSize=idata[0];
  hm->XSize=idata[1];

  hm->RowWidth=hm->XSize;
  hm->Scale=5;
  hm->Data = new float[hm->XSize * hm->YSize];
  float tmp;
  int range = (int) pow(2.0,8) - 1;
  for(int i=0; i < hm->XSize * hm->YSize; i++){
    fread((char *)&tmp,sizeof(float),1,fp);
    //if(isinf(tmp))
    //	    hm->Data[i]=100;//	    
    tmp=min;
    //	    hm->Data[i]=0;
    // else
    //	  else
    // hm->Data[i]=0;
    hm->Data[i]=((uint16)(((tmp-min)/zrange)*(range)));
    //printf("%f %f %f %d %d\n",tmp, (tmp-min)/zrange,zrange,range,hm->Data[i]);
  }
  hm->x_origin=24576;
  hm->y_origin=24576;

}

bool bound_grd( mesh_input &m,double &zmin, double &zmax,double local_easting, double local_northing){
  struct GRD_HEADER gmtheader;    /* GMT header to be written to file */
  int argc=1;
  const char *argv[]={"gmtiomodule",0};
  const int falsev=(1==0);
  argc=GMT_begin(argc,(char **)argv); /* Sets crazy globals */
  GMT_grd_init (&gmtheader, argc,(char **) argv, falsev); /* Initialize grd header structure  */
  if(GMT_read_grd_info ((char *)m.name.c_str(),&gmtheader) == -1){
    fprintf(stderr,"Cant open %s\n",m.name.c_str());
    return false;
  }

  if(gmtheader.type == -1 || gmtheader.nx == 0 || gmtheader.ny == 0){
   fprintf(stderr,"Cant open %s type appears invalid\n",m.name.c_str());
    return false;
  }
  UF::GeographicConversions::Redfearn gpsConversion;
  
  double gridConvergence, pointScale;
  std::string zone;
  double easting ,northing,l_xmin,l_ymin,l_xmax,l_ymax;
  gpsConversion.GetGridCoordinates(gmtheader.y_min,gmtheader.x_min,
				   zone, easting, northing, 
				   gridConvergence, pointScale);
  easting-=local_easting;
  northing-=local_northing;
   
  l_ymin=easting;
  l_xmin=northing;
 

  gpsConversion.GetGridCoordinates(gmtheader.y_max,gmtheader.x_max,
				   zone, easting, northing, 
				   gridConvergence, pointScale);
  easting-=local_easting;
  northing-=local_northing;
  
  l_ymax=easting;
  l_xmax=northing;
  
  m.envelope=Envelope<double>(l_xmin,l_ymin,
			      l_xmax,l_ymax);
  cout <<"sds"<< m.envelope<<endl; 
 const size_t nm = gmtheader.nx * gmtheader.ny;
 float *data;
 data = (float *) GMT_memory (VNULL, (size_t)nm, sizeof (float), GMT_program);

 /* Read the entire grd image */
 if (GMT_read_grd ((char*)m.name.c_str(), &gmtheader, data, 0.0, 0.0, 0.0, 0.0, GMT_pad, FALSE)) {
   // Bad?  free memory and bail
   printf ("IS THIS BAD?\n");
   GMT_free ((void *)data);

   return false;
 }
 
 for(int i=0; i < (int)nm; i++){
   if( data[i] < zmin)
     zmin= data[i];
   if( data[i] > zmax)
     zmax= data[i];
 }
 // std::cout << m.name << " " <<m.envelope << "zmin " << zmin << " zmax " << zmax<<std::endl;
 GMT_free ((void *)data);
 return true;

}

bool read_grd_data(const char *name,short &nx,short &ny,float *&data){
  struct GRD_HEADER gmtheader;    /* GMT header to be written to file */
  int argc=1;
  const char *argv[]={"gmtiomodule",0};
  const int falsev=(1==0);
  argc=GMT_begin(argc,(char **)argv); /* Sets crazy globals */
  GMT_grd_init (&gmtheader, argc,(char **) argv, falsev); /* Initialize grd header structure  */
 if(GMT_read_grd_info ((char *)name,&gmtheader) == -1 || gmtheader.type == -1 || gmtheader.nx == 0 || gmtheader.ny == 0){
   fprintf(stderr,"Cant open %s type appears invalid\n",name);
    return false;
  }
  nx=gmtheader.nx;  
  ny=gmtheader.ny;

  const size_t nm = gmtheader.nx * gmtheader.ny;

  data = (float *) GMT_memory (VNULL, (size_t)nm, sizeof (float), GMT_program);

 /* Read the entire grd image */
 if (GMT_read_grd ((char*)name, &gmtheader, data, 0.0, 0.0, 0.0, 0.0, GMT_pad, FALSE)) {
   // Bad?  free memory and bail
   printf ("IS THIS BAD?\n");
   GMT_free ((void *)data);

   return false;
 }
 
 return true;

}


