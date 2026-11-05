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
 10;
 1.40000;-0.50000;0.05000;,
 1.40000;0.50000;0.05000;,
 -1.40000;0.50000;0.05000;,
 -1.40000;-0.50000;0.05000;,
 1.40000;0.00000;-0.05000;,
 -1.40000;0.00000;-0.05000;,
 1.40000;0.00000;1.05000;,
 1.50000;0.00000;0.05000;,
 -1.50000;0.00000;0.05000;,
 -1.40000;0.00000;1.05000;;
 
 13;
 4;0,1,2,3;,
 4;4,5,2,1;,
 4;5,4,0,3;,
 3;0,6,1;,
 3;1,7,4;,
 3;1,6,7;,
 3;5,8,2;,
 3;2,8,9;,
 3;2,9,3;,
 3;0,7,6;,
 3;4,7,0;,
 3;3,9,8;,
 3;3,8,5;;
 
 MeshMaterialList {
  1;
  13;
  0,
  0,
  0,
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
   0.800000;0.401569;0.000000;1.000000;;
   5.000000;
   0.000000;0.000000;0.000000;;
   0.500000;0.250980;0.000000;;
  }
 }
 MeshNormals {
  13;
  0.000000;0.000000;1.000000;,
  0.000000;0.196116;-0.980581;,
  -1.000000;0.000000;0.000000;,
  0.700140;0.140028;-0.700140;,
  -0.700140;0.140028;-0.700140;,
  -0.975900;0.195180;0.097590;,
  0.000000;-0.196116;-0.980581;,
  0.975900;0.195180;0.097590;,
  1.000000;0.000000;-0.000000;,
  0.975900;-0.195180;0.097590;,
  0.700140;-0.140028;-0.700140;,
  -0.975900;-0.195180;0.097590;,
  -0.700140;-0.140028;-0.700140;;
  13;
  4;0,0,0,0;,
  4;1,1,1,1;,
  4;6,6,6,6;,
  3;2,2,2;,
  3;3,3,3;,
  3;7,7,7;,
  3;4,4,4;,
  3;5,5,5;,
  3;8,8,8;,
  3;9,9,9;,
  3;10,10,10;,
  3;11,11,11;,
  3;12,12,12;;
 }
 MeshTextureCoords {
  10;
  0.000000;0.000000;,
  0.000000;0.000000;,
  0.000000;0.000000;,
  0.000000;0.000000;,
  0.000000;0.000000;,
  0.000000;0.000000;,
  0.000000;0.000000;,
  0.000000;0.000000;,
  0.000000;0.000000;,
  0.000000;0.000000;;
 }
}
