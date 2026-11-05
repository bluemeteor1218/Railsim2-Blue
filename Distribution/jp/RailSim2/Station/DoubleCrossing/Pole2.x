xof 0302txt 0064
template Header {
 <3D82AB43-62DA-11cf-AB39-0020AF71E433>
 WORD major;
 WORD minor;
 DWORD flags;
}

template Vector {
 <3D82AB5E-62DA-11cf-AB39-0020AF71E433>
 FLOAT x;
 FLOAT y;
 FLOAT z;
}

template Coords2d {
 <F6F23F44-7686-11cf-8F52-0040333594A3>
 FLOAT u;
 FLOAT v;
}

template Matrix4x4 {
 <F6F23F45-7686-11cf-8F52-0040333594A3>
 array FLOAT matrix[16];
}

template ColorRGBA {
 <35FF44E0-6C7C-11cf-8F52-0040333594A3>
 FLOAT red;
 FLOAT green;
 FLOAT blue;
 FLOAT alpha;
}

template ColorRGB {
 <D3E16E81-7835-11cf-8F52-0040333594A3>
 FLOAT red;
 FLOAT green;
 FLOAT blue;
}

template IndexedColor {
 <1630B820-7842-11cf-8F52-0040333594A3>
 DWORD index;
 ColorRGBA indexColor;
}

template Boolean {
 <4885AE61-78E8-11cf-8F52-0040333594A3>
 WORD truefalse;
}

template Boolean2d {
 <4885AE63-78E8-11cf-8F52-0040333594A3>
 Boolean u;
 Boolean v;
}

template MaterialWrap {
 <4885AE60-78E8-11cf-8F52-0040333594A3>
 Boolean u;
 Boolean v;
}

template TextureFilename {
 <A42790E1-7810-11cf-8F52-0040333594A3>
 STRING filename;
}

template Material {
 <3D82AB4D-62DA-11cf-AB39-0020AF71E433>
 ColorRGBA faceColor;
 FLOAT power;
 ColorRGB specularColor;
 ColorRGB emissiveColor;
 [...]
}

template MeshFace {
 <3D82AB5F-62DA-11cf-AB39-0020AF71E433>
 DWORD nFaceVertexIndices;
 array DWORD faceVertexIndices[nFaceVertexIndices];
}

template MeshFaceWraps {
 <4885AE62-78E8-11cf-8F52-0040333594A3>
 DWORD nFaceWrapValues;
 Boolean2d faceWrapValues;
}

template MeshTextureCoords {
 <F6F23F40-7686-11cf-8F52-0040333594A3>
 DWORD nTextureCoords;
 array Coords2d textureCoords[nTextureCoords];
}

template MeshMaterialList {
 <F6F23F42-7686-11cf-8F52-0040333594A3>
 DWORD nMaterials;
 DWORD nFaceIndexes;
 array DWORD faceIndexes[nFaceIndexes];
 [Material]
}

template MeshNormals {
 <F6F23F43-7686-11cf-8F52-0040333594A3>
 DWORD nNormals;
 array Vector normals[nNormals];
 DWORD nFaceNormals;
 array MeshFace faceNormals[nFaceNormals];
}

template MeshVertexColors {
 <1630B821-7842-11cf-8F52-0040333594A3>
 DWORD nVertexColors;
 array IndexedColor vertexColors[nVertexColors];
}

template Mesh {
 <3D82AB44-62DA-11cf-AB39-0020AF71E433>
 DWORD nVertices;
 array Vector vertices[nVertices];
 DWORD nFaces;
 array MeshFace faces[nFaces];
 [...]
}

Header{
1;
0;
1;
}

Mesh {
 24;
 -0.02000;-0.00000;-0.50000;,
 -0.01000;0.01732;-0.50000;,
 0.01000;-0.01732;-0.50000;,
 -0.01000;-0.01732;-0.50000;,
 0.01000;0.01732;-0.50000;,
 0.02000;-0.00000;-0.50000;,
 -0.01000;0.01732;4.10000;,
 -0.02000;0.00000;4.10000;,
 -0.01000;-0.01732;4.10000;,
 0.01000;-0.01732;4.10000;,
 0.01000;0.01732;4.10000;,
 0.02000;0.00000;4.10000;,
 -0.02500;0.00000;-0.05000;,
 -0.02500;-0.04330;-0.02500;,
 -0.02500;-0.04330;0.02500;,
 -0.02500;0.00000;0.05000;,
 -0.02500;0.04330;-0.02500;,
 -0.02500;0.04330;0.02500;,
 0.07500;0.00000;0.05000;,
 0.07500;-0.04330;0.02500;,
 0.07500;-0.04330;-0.02500;,
 0.07500;0.00000;-0.05000;,
 0.07500;0.04330;0.02500;,
 0.07500;0.04330;-0.02500;;
 
 20;
 4;0,1,2,3;,
 4;1,4,5,2;,
 4;6,7,8,9;,
 4;10,6,9,11;,
 4;4,10,11,5;,
 4;1,6,10,4;,
 4;0,7,6,1;,
 4;3,8,7,0;,
 4;2,9,8,3;,
 4;5,11,9,2;,
 4;12,13,14,15;,
 4;16,12,15,17;,
 4;18,19,20,21;,
 4;22,18,21,23;,
 4;14,19,18,15;,
 4;13,20,19,14;,
 4;12,21,20,13;,
 4;16,23,21,12;,
 4;17,22,23,16;,
 4;15,18,22,17;;
 
 MeshMaterialList {
  2;
  20;
  1,
  1,
  1,
  1,
  1,
  1,
  1,
  1,
  1,
  1,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0;;
  Material {
   0.100000;0.100000;0.100000;1.000000;;
   5.000000;
   0.000000;0.000000;0.000000;;
   0.000000;0.000000;0.000000;;
  }
  Material {
   0.800000;0.800000;0.800000;1.000000;;
   5.000000;
   0.000000;0.000000;0.000000;;
   0.000000;0.000000;0.000000;;
   TextureFilename {
    "..\\SingleCrossing\\stripe.png";
   }
  }
 }
 MeshNormals {
  14;
  0.000000;0.000000;-1.000000;,
  0.000000;0.000000;1.000000;,
  -1.000000;0.000000;0.000000;,
  1.000000;0.000000;0.000000;,
  0.866032;0.499989;-0.000000;,
  0.000000;1.000000;0.000000;,
  -0.866032;0.499989;-0.000000;,
  -0.866032;-0.499989;0.000000;,
  0.000000;-1.000000;-0.000000;,
  0.866032;-0.499989;0.000000;,
  0.000000;-0.500000;0.866026;,
  0.000000;-0.500000;-0.866026;,
  0.000000;0.500000;-0.866026;,
  0.000000;0.500000;0.866026;;
  20;
  4;0,0,0,0;,
  4;0,0,0,0;,
  4;1,1,1,1;,
  4;1,1,1,1;,
  4;4,4,4,4;,
  4;5,5,5,5;,
  4;6,6,6,6;,
  4;7,7,7,7;,
  4;8,8,8,8;,
  4;9,9,9,9;,
  4;2,2,2,2;,
  4;2,2,2,2;,
  4;3,3,3,3;,
  4;3,3,3,3;,
  4;10,10,10,10;,
  4;8,8,8,8;,
  4;11,11,11,11;,
  4;12,12,12,12;,
  4;5,5,5,5;,
  4;13,13,13,13;;
 }
 MeshTextureCoords {
  24;
  0.400000;-2.000000;,
  0.450000;-2.000000;,
  0.550000;-2.000000;,
  0.450000;-2.000000;,
  0.550000;-2.000000;,
  0.600000;-2.000000;,
  0.450000;21.000000;,
  0.400000;21.000000;,
  0.450000;21.000000;,
  0.550000;21.000000;,
  0.550000;21.000000;,
  0.600000;21.000000;,
  -0.125000;5.500000;,
  -0.125000;5.608253;,
  -0.125000;5.608253;,
  -0.125000;5.500000;,
  -0.125000;5.391747;,
  -0.125000;5.391747;,
  2.375000;5.500000;,
  2.375000;5.608253;,
  2.375000;5.608253;,
  2.375000;5.500000;,
  2.375000;5.391747;,
  2.375000;5.391747;;
 }
}
