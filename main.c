#include <stdio.h>
#include <stdlib.h>
#include <math.h>

typedef unsigned long u64;
typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;
typedef long s64;
typedef int s32;
typedef short s16;
typedef char s8;

#define INDENT_SPACE "    "

#define LOG_OK printf(" OK\n")

void panic(const char* msg) {
    printf("\nPANIC: %s\nExiting ..\n", msg);
    exit(1);
}

#define TMD_HEADER_ID 0x00000041

typedef struct __attribute((packed)) {
    u32 id; // should match TMD_HEADER_ID
    u32 usesPointers;
    u32 objectCount;
} TmdFileHeader;

typedef struct __attribute((packed)) {
    u32 verticesOffset; // starts from file header
    u32 vertexCount;

    u32 normalsOffset; // starts from file header
    u32 normalCount;

    u32 primitivesOffset; // starts from file header
    u32 primitiveCount;

    s32 scale;
} TmdObjectHeader;

typedef struct __attribute((packed)) {
    s16 x, y, z;
    u16 _pad16;
} TmdVertex;

typedef struct __attribute((packed)) {
    u16 x, y, z;
    u16 _pad16;
} TmdNormal;

typedef struct {
    float x, y, z;
} WorkNormal;

// TmdNormals use 16-bit fixed-point values, so we need to convert them to floats
WorkNormal TmdNormalToWorkNormal(TmdNormal* tmdNormal) {
    WorkNormal workNormal;

    /*
        bit 15 | 14      12 | 11                                          0 |
        sign   | integral   | decimal                                       |
    */

    for (unsigned i = 0; i < 3; i++) {
        u16 fixedPoint = *((u16*)tmdNormal + i);
        float* floatingPoint = (float*)&workNormal + i;

        int signBit = (fixedPoint >> 15) & 0x1;
        int integralPart = (fixedPoint >> 12) & 0x7;
        int decimalPart = fixedPoint & 0xFFF;

        *floatingPoint = integralPart + (decimalPart / 4096.0f);

        if (signBit)
            *floatingPoint = -*floatingPoint;
    }

    return workNormal;
}

typedef struct __attribute((packed)) {
	u8 olen, ilen;
    u8 flag;
    u8 mode;
} TmdPrimitiveHeader;

#define IS_PRIM_POLYGON(primHeader) (((primHeader)->mode >> 5) & 1)

typedef struct __attribute((packed)) {
    u8 rgb[3]; // RGB color for whole triangle
    u8 _mode; // duplicate of mode

    u16 normalIndex; // index into normal table
    u16 vertexIndexes[3]; // indexes into vertex table
} TmdTriangleFlat;

typedef struct __attribute((packed)) {
    u8 rgb0[3]; // RGB color for vertex 0
    u8 _mode; // duplicate of header mode

    u8 rgb1[3]; // RGB color for vertex 1
    u8 _pad8_0;

    u8 rgb2[3]; // RGB color for vertex 2
    u8 _pad8_1;

    u16 normalIndex; // index into normal table
    u16 vertexIndexes[3]; // indexes into vertex table
} TmdTriangleGradated;

typedef struct __attribute((packed)) {
    u8 uv0[2]; // UV coordinates for vertex 0
    u16 cba; // CLUT number [ CBA clutY * 64 + clutX / 16) ]

    u8 uv1[2]; // UV coordinates for vertex 1
    u16 tsb; // Texture Page + Semitransparency Rate (0..3) << 5 + Colour Mode (0..2) << 7

    u8 uv2[2]; // UV coordinates for vertex 2
    u16 _pad16;

    u16 normalIndex; // index into normal table
    u16 vertexIndexes[3]; // indexes into vertex table
} TmdTriangleTextured;

typedef struct __attribute((packed)) {
    u8 rgb[3]; // RGB color for whole triangle
    u8 _mode; // duplicate of mode

    u16 nI0; // normal index for vertex 0
    u16 vI0; // vertex index for vertex 0

    u16 nI1; // normal index for vertex 1
    u16 vI1; // vertex index for vertex 1

    u16 nI2; // normal index for vertex 2
    u16 vI2; // vertex index for vertex 2
} TmdTriangleGouraud;

typedef struct __attribute((packed)) {
    u8 rgb0[3]; // RGB color for vertex 0
    u8 _mode; // duplicate of header mode

    u8 rgb1[3]; // RGB color for vertex 1
    u8 _pad8_0;

    u8 rgb2[3]; // RGB color for vertex 2
    u8 _pad8_1;

    u16 nI0; // normal index for vertex 0
    u16 vI0; // vertex index for vertex 0

    u16 nI1; // normal index for vertex 1
    u16 vI1; // vertex index for vertex 1

    u16 nI2; // normal index for vertex 2
    u16 vI2; // vertex index for vertex 2
} TmdTriangleGouraudGradated;

typedef struct __attribute((packed)) {
    u8 uv0[2]; // UV coordinates for vertex 0
    u16 cba; // position of CLUT in VRAM (use CBA_GET_CBX and CBA_GET_CBY)

    u8 uv1[2]; // UV coordinates for vertex 1
    u16 tsb; // Texture Page + Semitransparency Rate (0..3) << 5 + Colour Mode (0..2) << 7

    u8 uv2[2]; // UV coordinates for vertex 2
    u16 _pad16;

    u16 nI0; // normal index for vertex 0
    u16 vI0; // vertex index for vertex 0

    u16 nI1; // normal index for vertex 1
    u16 vI1; // vertex index for vertex 1

    u16 nI2; // normal index for vertex 2
    u16 vI2; // vertex index for vertex 2
} TmdTriangleGouraudTextured;

#define CBA_GET_CBX(cba) (((u16)cba >> 10) & 0x3F) // upper 6 bits
#define CBA_GET_CBY(cba) ((u16)cba & 0x1FF) // lower 9 bits

#define TSB_GET_TPAGE(tsb) ((u16)tsb & 0x1F) // bits 0-4
#define TSB_GET_ABR(tsb) (((u16)tsb >> 5) & 0x3) // bits 5-6
#define TSB_GET_TPF(tsb) (((u16)tsb >> 7) & 0x3) // bits 7-8

typedef struct __attribute((packed)) {
    u8 rgb[3]; // RGB color for whole triangle
    u8 _mode; // duplicate of mode

    u16 vertexIndexes[3]; // indexes into vertex table
    u16 _pad16;
} TmdTriangleNonlit;

typedef struct __attribute((packed)) {
    u8 uv0[2]; // UV coordinates for vertex 0
    u16 cba; // CLUT number [ CBA clutY * 64 + clutX / 16) ]

    u8 uv1[2]; // UV coordinates for vertex 1
    u16 tsb; // Texture Page + Semitransparency Rate (0..3) << 5 + Colour Mode (0..2) << 7

    u8 uv2[2]; // UV coordinates for vertex 2
    u16 _pad16_0;

    u8 rgb[3]; // Base color for whole triangle
    u8 _pad8;

    u16 vertexIndexes[3]; // indexes into vertex table
    u16 _pad16_1;
} TmdTriangleNonlitTextured;

typedef struct __attribute((packed)) {
    u8 uv0[2]; // UV coordinates for vertex 0
    u16 cba; // CLUT number [ CBA clutY * 64 + clutX / 16) ]

    u8 uv1[2]; // UV coordinates for vertex 1
    u16 tsb; // Texture Page + Semitransparency Rate (0..3) << 5 + Colour Mode (0..2) << 7

    u8 uv2[2]; // UV coordinates for vertex 2
    u16 _pad16_0;

    u8 rgb[3]; // Base color for whole triangle
    u8 _pad8;

    u16 vertexIndexes[3]; // indexes into vertex table
    u16 _pad16_1;
} TmdTriangleNonlitGouraud;

typedef struct __attribute((packed)) {
    u8 rgb[3]; // RGB color for whole line
    u8 _mode; // duplicate of mode

    u16 vertexIndexes[2]; // indexes into vertex table
} TmdLineFlat;

typedef struct __attribute((packed)) {
    u8 rgb0[3]; // RGB color for start of line
    u8 _mode; // duplicate of mode

    u8 rgb1[3]; // RGB color for end of line
    u8 _pad8;

    u16 vertexIndexes[2]; // indexes into vertex table
} TmdLineGradated;

#define HASH_PRIMITIVE_ATTRIBS(flag, mode) (((u32)(flag) * 0x65) + ((u32)(mode) * 0x65))

int main(int argc, char** argv) {
    if (argc < 2)
        panic("Missing input argument");

    printf("Read & copy binary ..");

    FILE* fpBin = fopen(argv[1], "rb");
    if (fpBin == NULL)
        panic("The binary could not be opened.");

    fseek(fpBin, 0, SEEK_END);
    u64 bufSize = ftell(fpBin);
    rewind(fpBin);

    u8* buffer = (u8 *)malloc(bufSize);
    if (buffer == NULL) {
        fclose(fpBin);

        panic("Failed to allocate bin buf");
    }

    u64 bytesCopied = fread(buffer, 1, bufSize, fpBin);
    if (bytesCopied != bufSize) {
        free(buffer);
        fclose(fpBin);

        panic("Buffer readin fail");
    }

    fclose(fpBin);

    LOG_OK;

    TmdFileHeader* fileHeader = (TmdFileHeader*)buffer;
    if (fileHeader->id != TMD_HEADER_ID)
        panic("File header ID is nonmatching");

    printf("\n-- TMD at path '%s' --\n", argv[1]);

    printf("! Uses offsets / pointers: %s\n", fileHeader->usesPointers ? "pointers" : "offsets");
    printf("! Object count: %u\n", fileHeader->objectCount);

    if (!fileHeader->usesPointers) {
        TmdObjectHeader* objects = (TmdObjectHeader*)(fileHeader + 1);

        for (unsigned i = 0; i < fileHeader->objectCount; i++) {
            printf("\n- Object no. %u:\n", i+1);

            TmdObjectHeader* objectHeader = objects + i;

            TmdVertex* vertices =
                (TmdVertex*)((u8*)(fileHeader + 1) + objectHeader->verticesOffset);
            TmdNormal* normals =
                (TmdNormal*)((u8*)(fileHeader + 1) + objectHeader->normalsOffset);

            /*
            printf("* Vertices:\n");
            for (unsigned j = 0; j < objectHeader->vertexCount; j++) {
                TmdVertex* vertex = vertices + j;
                printf(INDENT_SPACE "Vrtx %u: [%d, %d, %d]\n", j+1, vertex->x, vertex->y, vertex->z);
            }

            printf("* Normals:\n");
            for (unsigned j = 0; j < objectHeader->normalCount; j++) {
                TmdNormal* normal = normals + j;
                printf(INDENT_SPACE "Nrml %u: [%d, %d, %d]\n", j+1, normal->x, normal->y, normal->z);
            }
            */

            printf("* Scale = %f\n", powf(2.f, (float)objectHeader->scale));

            printf("* Vertices (%u)\n", objectHeader->vertexCount);
            printf("* Normals (%u)\n", objectHeader->normalCount);

            printf("* Primitives (%u):\n", objectHeader->primitiveCount);

            void* primitiveSectionStart = (u8*)(fileHeader + 1) + objectHeader->primitivesOffset;

            TmdPrimitiveHeader* currentPrimitiveHeader = (TmdPrimitiveHeader*)primitiveSectionStart;
            for (unsigned j = 0; j < objectHeader->primitiveCount; j++) {
                void* currentPrimitiveData = (void*)(currentPrimitiveHeader + 1);

                int isPolygon = IS_PRIM_POLYGON(currentPrimitiveHeader);

                printf(
                    INDENT_SPACE "%u. Prim %s (flag = %u, mode = %u):\n",
                    j+1,
                    isPolygon ? "Polygon" : "Line",
                    (u32)currentPrimitiveHeader->flag,
                    (u32)currentPrimitiveHeader->mode
                );

                // TODO: implement other prim types

                switch (HASH_PRIMITIVE_ATTRIBS(currentPrimitiveHeader->flag, currentPrimitiveHeader->mode)) {
                case HASH_PRIMITIVE_ATTRIBS(0, 0x20): {
                    TmdTriangleFlat* tri = currentPrimitiveData;

                    TmdVertex* v0 = vertices + tri->vertexIndexes[0];
                    TmdVertex* v1 = vertices + tri->vertexIndexes[1];
                    TmdVertex* v2 = vertices + tri->vertexIndexes[2];

                    printf(
                        INDENT_SPACE INDENT_SPACE "Triangle (Flat)\n"
                        INDENT_SPACE INDENT_SPACE "RGB{%03u %03u %03u},\n"
                        INDENT_SPACE INDENT_SPACE "V0{%hi, %hi, %hi}, V1{%hi, %hi, %hi}, V2{%hi, %hi, %hi}\n",
                        (u32)tri->rgb[0], (u32)tri->rgb[1], (u32)tri->rgb[2],
                        v0->x, v0->y, v0->z, v1->x, v1->y, v1->z, v2->x, v2->y, v2->z
                    );
                } break;

                case HASH_PRIMITIVE_ATTRIBS(0, 0x30): {
                    TmdTriangleGouraud* tri = currentPrimitiveData;

                    TmdVertex* v0 = vertices + tri->vI0;
                    TmdVertex* v1 = vertices + tri->vI1;
                    TmdVertex* v2 = vertices + tri->vI2;

                    WorkNormal n0 = TmdNormalToWorkNormal(normals + tri->nI0);
                    WorkNormal n1 = TmdNormalToWorkNormal(normals + tri->nI1);
                    WorkNormal n2 = TmdNormalToWorkNormal(normals + tri->nI2);

                    printf(
                        INDENT_SPACE INDENT_SPACE "Triangle (Gouraud)\n"
                        INDENT_SPACE INDENT_SPACE "RGB{%03u %03u %03u},\n"
                        INDENT_SPACE INDENT_SPACE "N0{%f, %f, %f}, N1{%f, %f, %f}, N2{%f, %f, %f},\n"
                        INDENT_SPACE INDENT_SPACE "V0{%hi, %hi, %hi}, V1{%hi, %hi, %hi}, V2{%hi, %hi, %hi}\n",
                        (u32)tri->rgb[0], (u32)tri->rgb[1], (u32)tri->rgb[2],
                        n0.x, n0.y, n0.z, n1.x, n1.y, n1.z, n2.x, n2.y, n2.z,
                        v0->x, v0->y, v0->z, v1->x, v1->y, v1->z, v2->x, v2->y, v2->z
                    );
                } break;
                
                case HASH_PRIMITIVE_ATTRIBS(0, 0x40):
                case HASH_PRIMITIVE_ATTRIBS(1, 0x40): {
                    TmdLineFlat* line = currentPrimitiveData;

                    TmdVertex* v0 = vertices + line->vertexIndexes[0];
                    TmdVertex* v1 = vertices + line->vertexIndexes[1];

                    printf(
                        INDENT_SPACE INDENT_SPACE "Line\n"
                        INDENT_SPACE INDENT_SPACE "RGB{%03u %03u %03u},\n"
                        INDENT_SPACE INDENT_SPACE "V0{%hi, %hi, %hi}, V1{%hi, %hi, %hi}\n",
                        (u32)line->rgb[0], (u32)line->rgb[1], (u32)line->rgb[2],
                        v0->x, v0->y, v0->z, v1->x, v1->y, v1->z
                    );
                } break;

                case HASH_PRIMITIVE_ATTRIBS(1, 0x21): {
                    TmdTriangleNonlit* tri = currentPrimitiveData;

                    TmdVertex* v0 = vertices + tri->vertexIndexes[0];
                    TmdVertex* v1 = vertices + tri->vertexIndexes[1];
                    TmdVertex* v2 = vertices + tri->vertexIndexes[2];

                    printf(
                        INDENT_SPACE INDENT_SPACE "Triangle (Flat, Non-lit)\n"
                        INDENT_SPACE INDENT_SPACE "RGB{%03u %03u %03u},\n"
                        INDENT_SPACE INDENT_SPACE "V0{%hi, %hi, %hi}, V1{%hi, %hi, %hi}, V2{%hi, %hi, %hi}\n",
                        (u32)tri->rgb[0], (u32)tri->rgb[1], (u32)tri->rgb[2],
                        v0->x, v0->y, v0->z, v1->x, v1->y, v1->z, v2->x, v2->y, v2->z
                    );
                } break;

                case HASH_PRIMITIVE_ATTRIBS(1, 0x25): {
                    TmdTriangleNonlitTextured* tri = currentPrimitiveData;

                    TmdVertex* v0 = vertices + tri->vertexIndexes[0];
                    TmdVertex* v1 = vertices + tri->vertexIndexes[1];
                    TmdVertex* v2 = vertices + tri->vertexIndexes[2];

                    u32 cbx = CBA_GET_CBX(tri->cba);
                    u32 cby = CBA_GET_CBY(tri->cba);

                    char* ABRS;
                    switch (TSB_GET_ABR(tri->tsb)) {
                    case 0:
                        ABRS = "0.5 back + 0.5 poly";
                        break;
                    case 1:
                        ABRS = "1.0 back + 1.0 poly";
                    case 2:
                        ABRS = "1.0 back - 1.0 poly";
                    case 3:
                        ABRS = "1.0 back + 0.25 poly";
                    default:
                        ABRS = "Invalid";
                        break;
                    }

                    char* TPFS;
                    switch (TSB_GET_TPF(tri->tsb)) {
                    case 0:
                        TPFS = "4bit";
                        break;
                    case 1:
                        TPFS = "8bit";
                    case 2:
                        TPFS = "15bit";
                    default:
                        TPFS = "Invalid";
                        break;
                    }

                    printf(
                        INDENT_SPACE INDENT_SPACE "Triangle (Textured, Non-lit)\n"
                        INDENT_SPACE INDENT_SPACE "UV0{%03u %03u}, UV1{%03u %03u}, UV2{%03u %03u},\n"
                        INDENT_SPACE INDENT_SPACE "CLUT{%03u %03u}\n"
                        INDENT_SPACE INDENT_SPACE "TPAGE = %u\n"
                        INDENT_SPACE INDENT_SPACE "TRANSPARENCY = %s\n"
                        INDENT_SPACE INDENT_SPACE "COLOR = %s\n"
                        INDENT_SPACE INDENT_SPACE "RGB{%03u %03u %03u},\n"
                        INDENT_SPACE INDENT_SPACE "V0{%hi, %hi, %hi}, V1{%hi, %hi, %hi}, V2{%hi, %hi, %hi}\n",
                        (u32)tri->uv0[0], (u32)tri->uv0[1], (u32)tri->uv1[0], (u32)tri->uv1[1],
                        (u32)tri->uv2[0], (u32)tri->uv2[1],
                        cbx, cby,
                        TSB_GET_TPAGE(tri->tsb),
                        ABRS,
                        TPFS,
                        (u32)tri->rgb[0], (u32)tri->rgb[1], (u32)tri->rgb[2],
                        v0->x, v0->y, v0->z, v1->x, v1->y, v1->z, v2->x, v2->y, v2->z
                    );
                } break;

                default:
                    printf(
                        INDENT_SPACE INDENT_SPACE "Unknown (ilen = %u, olen = %u)\n",
                        (u32)currentPrimitiveHeader->ilen,
                        (u32)currentPrimitiveHeader->olen
                    );
                    break;
                }

                currentPrimitiveHeader = (TmdPrimitiveHeader*)(
                    (u8*)(currentPrimitiveHeader + 1) + (currentPrimitiveHeader->ilen * 4)
                );
            }
        }
    }

    free(buffer);

    printf("\nAll done. Exiting..\n");

    return 0;
}