# projet HPC

le but de se projet est d'améliré les performances de [tinyrenderer](https://github.com/ssloy/tinyrenderer), il s'agit d'un OpenGL simplifier a des buts de formation. Je me met comme defis de pouvoir garder le code aussi simple que possible afin de respecter l'ésprit du repo original, peu être en écrivant deux versions d'une fonction pour une "normal" et une "optimiser" afin qu'il soit facile de comprendre comment amélioré tels partie de code toujours a des fin de formation.

## méthodologie

J'utilise le main orginal du repo qui construit un fichier tga avec l'obj passer en argument. j'utilise les obj fourni dans le fichier obj du repo pour me servir de marque de référence pour les performances. je lance donc le main des façon suivante : 

./main obj/african_head/african_head.obj 

./main obj/boggie/body.obj  

./main ./obj/diablo3_pose/diablo3_pose.obj 

pour mesurer la performance  perf stat 

```sh
sudo perf stat -e cycles,cpu-clock,faults,cache-misses,branch-misses,migrations,cs ./main obj/model.obj
```

## african head

Performance counter stats for './main obj/african_head/african_head.obj':

     4'386'492'382      cycles                    #    1.592 GHz                      (66.45%)
          2'754.96 msec cpu-clock                 #    0.996 CPUs utilized          
             3'097      faults                    #    0.001 M/sec                  
           175'219      cache-misses                                                  (66.78%)
        12'532'215      branch-misses                                                 (66.77%)
                 0      migrations                #    0.000 K/sec                  
               293      cs                        #    0.106 K/sec                  
    
       2.766496593 seconds time elapsed
    
       2.739946000 seconds user
       0.015976000 seconds sys
## bogy boddie 

 Performance counter stats for './main obj/boggie/body.obj':

     4'501'055'186      cycles                    #    1.592 GHz                      (66.68%)
          2'827.21 msec cpu-clock                 #    0.997 CPUs utilized          
            10'533      faults                    #    0.004 M/sec                  
           726'989      cache-misses                                                  (66.68%)
        10'913'623      branch-misses                                                 (66.64%)
                 0      migrations                #    0.000 K/sec                  
               252      cs                        #    0.089 K/sec                  
    
       2.834969285 seconds time elapsed
    
       2.775543000 seconds user
       0.051916000 seconds sys
## diabo 3 pose

 Performance counter stats for './main ./obj/diablo3_pose/diablo3_pose.obj':

     4'142'533'244      cycles                    #    1.591 GHz                      (66.57%)
          2'603.20 msec cpu-clock                 #    0.991 CPUs utilized          
             3'694      faults                    #    0.001 M/sec                  
           288'535      cache-misses                                                  (66.78%)
        11'232'366      branch-misses                                                 (66.64%)
                 1      migrations                #    0.000 K/sec                  
               282      cs                        #    0.108 K/sec                  
    
       2.627876732 seconds time elapsed
    
       2.572036000 seconds user
       0.031950000 seconds sys
afin de m'aider a trouver les points d'amélioration possible j'utilise perf report. 

# amélioration des performances 

## utlisation des bibliothéques approprier 

Comme souvent dans les languages très utilisé des bibliothèques extrêmement bien optimiser existe. dans notre cas nous voyons dans le main un remplissage de tableau peu orthodoxe fait de la manière suivante : 

```c++
float *zbuffer = new float[width*height];
for (int i=width*height; i--; zbuffer[i] = -std::numeric_limits<float>::max());
```

Nous allons simplement utilisé la bibliothèque algorithm avec la méthode fill_n pour remplire ce buffer avec la valeur -- std::numeric_limits<float>::max()) comme ceci :

```c++
float *zbuffer = new float[width*height];
std::fill_n(zbuffer,width*height, -std::numeric_limits<float>::max());
```

en ajoutant au préalable la librairie algorithm.

### resultat de cette opération 

nous n'effectuons les résultats intermediaire que sur l'une des trois générations d'image afin de nous économiser du temps en fin de rapport le tableau comparatif sera effectuer pour les 3.

#### diablio 

 Performance counter stats for './main ./obj/diablo3_pose/diablo3_pose.obj':

     4'128'450'531      cycles                    #    1.592 GHz                      (66.65%)
          2'593.48 msec cpu-clock                 #    0.998 CPUs utilized          
             3'694      faults                    #    0.001 M/sec                  
           304'354      cache-misses                                                  (66.78%)
        11'747'138      branch-misses                                                 (66.57%)
                 0      migrations                #    0.000 K/sec                  
               231      cs                        #    0.089 K/sec                  
    
       2.597754811 seconds time elapsed
    
       2.582094000 seconds user
       0.011991000 seconds sys
|       |   cycles   | cpu-clock | faults | cache-misses | branch-misses | migrations | seconds time elapsed | seconds user | seconds sys |
| :---: | :--------: | :-------: | :----: | :----------: | :-----------: | :--------: | :------------------: | :----------: | :---------: |
| diabo | -4'819'062 |   -1.15   |  + 1   |   -46'756    |   -697'047    |    + 1     |     −0.010849296     |   - 0.0009   |      0      |

## fonction barycentrique

lorsqu'on utilise perf ou valgrind on voit que la fonction barycentrique provoque énormément de cache-misses/branch-misses. Cella proviens du fait que le programme est très orienté objet et donc le compilateur ne peux pas beaucoup optimiser ce code. En gardant la simplicité d'utilisation de la fonction (on gardera la même signature), nous allons effectuer les opérations "directement", remarquer que u[2] peu être la seul cordonné du vecteur calculer étant donné que si le triangle est retourné par rapport a la caméra il n'est pas afficher. 

```c++
Vec3f barycentric(Vec2f A, Vec2f B, Vec2f C, Vec2f P) {
    float u[3];
    u[2] = (C[0] - A[0]) * (B[1] - A[1]) - (B[0] - A[0]) * (C[1] - A[1]);
    if (std::abs(u[2]) <= 1e-2) {
        return Vec3f(-1, 1, 1);
    } else {
        u[0] = (B[0] - A[0]) * (A[1] - P[1]) - (A[0] - P[0]) * (B[1] - A[1]);
    	u[1] = (A[0] - P[0]) * (C[1] - A[1]) - (C[0] - A[0]) * (A[1] - P[1]);
        return Vec3f(1.f - (u[0] + u[1]) / u[2], u[1] / u[2], u[0] / u[2]);
    }
}
```

## gain de performance

regardons la différence de performence apporté apporté :

 Performance counter stats for './main ./obj/diablo3_pose/diablo3_pose.obj':

     4'040'474'978      cycles                    #    1.592 GHz                      (66.44%)
          2'537.62 msec cpu-clock                 #    0.993 CPUs utilized          
             3'696      faults                    #    0.001 M/sec                  
           245'713      cache-misses                                                  (66.91%)
        10'227'605      branch-misses                                                 (66.65%)
                 2      migrations                #    0.001 K/sec                  
               276      cs                        #    0.109 K/sec                  
    
       2.556256736 seconds time elapsed
    
       2.518512000 seconds user
       0.019956000 seconds sys
on voit que la différence apporté est minime pour faire mieux nous allons essayer avec des Single Instruction on Multiple Data comme ceci sachant bien que cette fois ci on effectuera toujours le calcul baricentrique même si on voit que le vecteur normal n'est pas face à la caméra : 

```c++
Vec3f barycentric(Vec2f A, Vec2f B, Vec2f C, Vec2f P) {

    __m128 s1 = _mm_set_ps(0, C[0], A[0], B[0]);
    __m128 s2 = _mm_set_ps(0, A[0], P[0], A[0]);
    s1 = _mm_sub_ps(s1, s2); // [(B[0] - A[0]),(A[0] - P[0]),(C[0] - A[0]),0]

    __m128 s3 = _mm_set_ps(0, B[1], C[1], A[1]);
    __m128 s4 = _mm_set_ps(0, A[1], A[1], P[1]);
    s3 = _mm_sub_ps(s3, s4);// [(A[1] - P[1]),(C[1] - A[1]),(B[1] - A[1]),0]

    __m128 s5 = _mm_set_ps(0, B[0], C[0], A[0]);
    __m128 s6 = _mm_set_ps(0, A[0], A[0], P[0]);
    s5 = _mm_sub_ps(s5, s6);// [(A[0] - P[0]),(C[0] - A[0]),(B[0] - A[0])]

    __m128 s7 = _mm_set_ps(0, C[1], A[1], B[1]);
    __m128 s8 = _mm_set_ps(0, A[1], P[1], A[1]);
    s7 = _mm_sub_ps(s7, s8);//[(B[1] - A[1]),(A[1] - P[1]),(C[1] - A[1]),0]

    s1 = _mm_mul_ps(s1, s3);//s1*s3
    s5 = _mm_mul_ps(s5, s7);// s5*s7
    s1 = _mm_sub_ps(s1, s5); // s1-s5
    
    float u[4];
    _mm_store_ps(u, s1);
    if (std::abs(u[2]) <= 1e-2) {
        return Vec3f(-1, 1, 1);
    } else {
        return Vec3f(1.f - (u[0] + u[1]) / u[2], u[1] / u[2], u[0] / u[2]);
    }

}
```

### amélioration de performance :

 Performance counter stats for './main ./obj/diablo3_pose/diablo3_pose.obj':

     4'209'653'977      cycles                    #    1.593 GHz                      (66.73%)
          2'643.30 msec cpu-clock                 #    0.996 CPUs utilized          
             3'695      faults                    #    0.001 M/sec                  
           268'761      cache-misses                                                  (66.50%)
        13'845'690      branch-misses                                                 (66.77%)
                 0      migrations                #    0.000 K/sec                  
               267      cs                        #    0.101 K/sec                  
    
       2.652869555 seconds time elapsed
    
       2.619942000 seconds user
       0.023962000 seconds sys
on voit ici que ce n'est pas une bonne idée d'utilisé des SIMD , le fait de ne pas faire tout les calculs est beaucoup plus rentable, cela nous enlève des caches-misses très gourmante en terme de temps d'excecution.