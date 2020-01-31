/**********************************************************
 *
 *                 readply.h
 *
 *  Declarations of the data structures used by the readply
 *  procedure.  This includes the information returned by
 *  the procedure.
 *
 **********************************************************/
 
 /*
  *  The vertex struct contains the information
  *  for a single vertex.  This includes its x,
  *  y and z coordinates.  The vertex table is
  *  an array of these structs.
  */
 struct ply_vertex {
        float x, y, z;
 };
 
 /*
  *  The ply_face struct contains all the information
  *  for a single face or polygon in the model.  The n
  *  field contains the number of vertices in the face,
  *  and the vertices field is an array of vertex
  *  table indices.  The face table is an array of
  *  these structs.
  */
 struct ply_face {
        unsigned char n;
        int *vertices;
};

/*
 * The ply_model struct is the value returned by the
 * readply functon.  This structure contains all the
 * information in the model.  The nvertex field is the
 * number of vertices in the vertex table.  The vertices
 * field is the vertex table.  The nface field is the
 * number of entries in the face table.  The faces
 * field is the faces table.
 */
struct ply_model {
       int nvertex;
       struct ply_vertex *vertices;
       int nface;
       struct ply_face *faces;
};

/*
 * The declaration of the readply function.  The parameter
 * to this function is the name of the model file, and the
 * return value is a pointer to the model read from the file.
 */
struct ply_model *readply(char *filename);
        
