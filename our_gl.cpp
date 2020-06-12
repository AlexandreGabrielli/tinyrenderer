#include <cmath>
#include <limits>
#include <cstdlib>
#include "our_gl.h"

#include <xmmintrin.h>
#include <xmmintrin.h> // SSE
#include <emmintrin.h> // SSE2
#include <pmmintrin.h> // SSE3
#include <tmmintrin.h> // SSSE3
#include <smmintrin.h> // SSE4.1
#include <nmmintrin.h> // SSE4.2

Matrix ModelView;
Matrix Viewport;
Matrix Projection;

IShader::~IShader() {}

void viewport(int x, int y, int w, int h) {
    Viewport = Matrix::identity();
    Viewport[0][3] = x+w/2.f;
    Viewport[1][3] = y+h/2.f;
    Viewport[2][3] = 1.f;
    Viewport[0][0] = w/2.f;
    Viewport[1][1] = h/2.f;
    Viewport[2][2] = 0;
}

void projection(float coeff) {
    Projection = Matrix::identity();
    Projection[3][2] = coeff;
}

void lookat(Vec3f eye, Vec3f center, Vec3f up) {
    Vec3f z = (eye-center).normalize();
    Vec3f x = cross(up,z).normalize();
    Vec3f y = cross(z,x).normalize();
    Matrix Minv = Matrix::identity();
    Matrix Tr   = Matrix::identity();
    for (int i=0; i<3; i++) {
        Minv[0][i] = x[i];
        Minv[1][i] = y[i];
        Minv[2][i] = z[i];
        Tr[i][3] = -center[i];
    }
    ModelView = Minv*Tr;
}

Vec3f barycentric(Vec2f A, Vec2f B, Vec2f C, Vec2f P) {
/**
//        u[0] = (B[0] - A[0]) * ______ - ______ * ________;
//        u[1] = (A[0] - P[0]) * ______ - ______ * ________;
//        u[2] = (C[0] - A[0]) * ______ - ______ * ________;
    __m128 s1 = _mm_set_ps(B[0],A[0] , C[0],0);
    __m128 s2 = _mm_set_ps(A[0],P[0] , A[0],0);
   s1 = _mm_sub_ps(s1 ,s2);
    
//        u[0] = ______ * (A[1] - P[1]) - ______ * ________;
//        u[1] = ______ * (C[1] - A[1]) - ______ * ________;
//        u[2] = ______ * (B[1] - A[1]) - ______ * ________;
    __m128 s3 = _mm_set_ps(A[1],C[1] , B[1],0);
    __m128 s4 = _mm_set_ps(P[1],A[1] , A[1],0);
    s3= _mm_sub_ps(s3 ,s4);

//        u[0] = ______ * ______ - (A[0] - P[0]) * ______;
//        u[1] = ______ * ______ - (C[0] - A[0]) * ______;
//        u[2] = ______ * ______ - (B[0] - A[0]) * ______;

    __m128 s5 = _mm_set_ps(A[0],C[0] , B[0],0);
    __m128 s6 = _mm_set_ps(P[0],A[0] , A[0],0);
    s5 = _mm_sub_ps(s5 ,s6);

//        u[0] = ______ * ______ - ______ * (B[1] - A[1]);
//        u[1] = ______ * ______ - ______ * (A[1] - P[1]);
//        u[2] = ______ * ______ - ______ * (C[1] - A[1]);
    __m128 s7 = _mm_set_ps(B[1],A[1] , C[1],0);
    __m128 s8 = _mm_set_ps(A[1],P[1] , A[1],0);
    s7 = _mm_sub_ps(s7 ,s8);
    
//        u[0] = s1 * s3 - ______ * ______;
//        u[1] = s1 * s3 - ______ * ______;
//        u[2] = s1 * s3 - ______ * ______;
    s1 = _mm_mul_ps(s1,s3);

//        u[0] = ______ * ______ - s5 * s7;
//        u[1] = ______ * ______ - s5 * s7;
//        u[2] = ______ * ______ - s5 * s7;
    s5 = _mm_mul_ps(s5,s7);

//        u[0] = s1 - s5;
//        u[1] = s1 - s5;
//        u[2] = s1 - s5;
    s1 = _mm_sub_ps(s1 ,s5);

    float u[4];
    
    _mm_store_ps(u , s1);
    if (std::abs(u[2]) <= 1e-2) {
        return Vec3f(-1, 1, 1);
        } else {
        return Vec3f (1.f - (u[0] + u[1]) / u[2], u[1] / u[2], u[0] / u[2]);
    }
*/	

        float u[4];
        u[0] = (B[0] - A[0]) * (A[1] - P[1]) - (A[0] - P[0]) * (B[1] - A[1]);
        u[1] = (A[0] - P[0]) * (C[1] - A[1]) - (C[0] - A[0]) * (A[1] - P[1]);
        u[2] = (C[0] - A[0]) * (B[1] - A[1]) - (B[0] - A[0]) * (C[1] - A[1]);
    if (std::abs(u[2]) <= 1e-2) {
        return Vec3f(-1, 1, 1);
    } else {
       return Vec3f (1.f - (u[0] + u[1]) / u[2], u[1] / u[2], u[0] / u[2]);
     }


}


void triangle(mat<4,3,float> &clipc, IShader &shader, TGAImage &image, float *zbuffer) {
    mat<3,4,float> pts  = (Viewport*clipc).transpose(); // transposed to ease access to each of the points
    mat<3,2,float> pts2;
    for (int i=0; i<3; i++) pts2[i] = proj<2>(pts[i]/pts[i][3]);

    Vec2f bboxmin( std::numeric_limits<float>::max(),  std::numeric_limits<float>::max());
    Vec2f bboxmax(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());
    Vec2f clamp(image.get_width()-1, image.get_height()-1);
    for (int i=0; i<3; i++) {
        for (int j=0; j<2; j++) {
            bboxmin[j] = std::max(0.f,      std::min(bboxmin[j], pts2[i][j]));
            bboxmax[j] = std::min(clamp[j], std::max(bboxmax[j], pts2[i][j]));
        }
    }
    Vec2i P;
    TGAColor color;
    for (P.x=bboxmin.x; P.x<=bboxmax.x; P.x++) {
        for (P.y=bboxmin.y; P.y<=bboxmax.y; P.y++) {
            Vec3f bc_screen  = barycentric(pts2[0], pts2[1], pts2[2], P);
            Vec3f bc_clip    = Vec3f(bc_screen.x/pts[0][3], bc_screen.y/pts[1][3], bc_screen.z/pts[2][3]);
            bc_clip = bc_clip/(bc_clip.x+bc_clip.y+bc_clip.z);
            float frag_depth = clipc[2]*bc_clip;
            if (bc_screen.x<0 || bc_screen.y<0 || bc_screen.z<0 || zbuffer[P.x+P.y*image.get_width()]>frag_depth) continue;
            bool discard = shader.fragment(bc_clip, color);
            if (!discard) {
                zbuffer[P.x+P.y*image.get_width()] = frag_depth;
                image.set(P.x, P.y, color);
            }
        }
    }
}

