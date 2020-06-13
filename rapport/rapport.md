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

dans ce programme on calcul très souvent le barycentre d'un triangle que ce soit pour 



 Performance counter stats for './main ./obj/boggie/body.obj':

     4'423'650'442      cycles                    #    1.592 GHz                      (66.54%)
          2'779.19 msec cpu-clock                 #    0.989 CPUs utilized          
            10'532      faults                    #    0.004 M/sec                  
           703'178      cache-misses                                                  (66.83%)
        10'360'565      branch-misses                                                 (66.64%)
                 6      migrations                #    0.002 K/sec                  
               501      cs                        #    0.180 K/sec                  
    
       2.810681057 seconds time elapsed
    
       2.708109000 seconds user
       0.072002000 seconds sys