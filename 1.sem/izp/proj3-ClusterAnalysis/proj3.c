/**
 * Kostra programu pro 3. projekt IZP 2015/16
 *
 * Jednoducha shlukova analyza
 * Complete linkage
 * http://is.muni.cz/th/172767/fi_b/5739129/web/web/clsrov.html
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h> // sqrtf
#include <limits.h> // INT_MAX

/*****************************************************************
 * Ladici makra. Vypnout jejich efekt lze definici makra
 * NDEBUG, napr.:
 *   a) pri prekladu argumentem prekladaci -DNDEBUG
 *   b) v souboru (na radek pred #include <assert.h>
 *      #define NDEBUG
 */
#ifdef NDEBUG
#define debug(s)
#define dfmt(s, ...)
#define dint(i)
#define dfloat(f)
#else

// vypise ladici retezec
#define debug(s) printf("- %s\n", s)

// vypise formatovany ladici vystup - pouziti podobne jako printf
#define dfmt(s, ...) printf(" - "__FILE__":%u: "s"\n",__LINE__,__VA_ARGS__)

// vypise ladici informaci o promenne - pouziti dint(identifikator_promenne)
#define dint(i) printf(" - " __FILE__ ":%u: " #i " = %d\n", __LINE__, i)

// vypise ladici informaci o promenne typu float - pouziti
// dfloat(identifikator_promenne)
#define dfloat(f) printf(" - " __FILE__ ":%u: " #f " = %g\n", __LINE__, f)

#endif

/*****************************************************************
 * Deklarace potrebnych datovych typu:
 *
 * TYTO DEKLARACE NEMENTE
 *
 *   struct obj_t - struktura objektu: identifikator a souradnice
 *   struct cluster_t - shluk objektu:
 *      pocet objektu ve shluku,
 *      kapacita shluku (pocet objektu, pro ktere je rezervovano
 *          misto v poli),
 *      ukazatel na pole shluku.
 */

struct obj_t {
    int id;
    float x;
    float y;
};

struct cluster_t {
    int size;
    int capacity;
    struct obj_t *obj;
};

/*****************************************************************
 * Deklarace potrebnych funkci.
 *
 * PROTOTYPY FUNKCI NEMENTE
 *
 * IMPLEMENTUJTE POUZE FUNKCE NA MISTECH OZNACENYCH 'TODO'
 *
 */

/*
 Inicializace shluku 'c'. Alokuje pamet pro cap objektu (kapacitu).
 Ukazatel NULL u pole objektu znamena kapacitu 0.
*/
void init_cluster(struct cluster_t *c, int cap)
{
    assert(c != NULL);
    assert(cap >= 0);

    // TODO

    c->obj = malloc( cap * sizeof(struct obj_t));
    if (&(c->obj) == NULL)
    {
      c->capacity = 0;
    }
    else
    {
      c->capacity = cap;
    }
}

/*
 Odstraneni vsech objektu shluku a inicializace na prazdny shluk.
 */
void clear_cluster(struct cluster_t *c)
{
    // TODO

    free(c->obj);

    assert (c->obj == NULL);

    c->size = 0;
    c->capacity = 0;
}

/// Chunk of cluster myObjects. Value recommended for reallocation.
const int CLUSTER_CHUNK = 10;

/*
 Zmena kapacity shluku 'c' na kapacitu 'new_cap'.
 */
struct cluster_t *resize_cluster(struct cluster_t *c, int new_cap)
{
    // TUTO FUNKCI NEMENTE
    assert(c);
    assert(c->capacity >= 0);
    assert(new_cap >= 0);

    if (c->capacity >= new_cap)
        return c;

    size_t size = sizeof(struct obj_t) * new_cap;

    void *arr = realloc(c->obj, size);
    if (arr == NULL)
        return NULL;

    c->obj = arr;
    c->capacity = new_cap;
    return c;
}

/*
 Prida objekt 'obj' na konec shluku 'c'. Rozsiri shluk, pokud se do nej objekt
 nevejde.
 */
void append_cluster(struct cluster_t *c, struct obj_t obj)
{
    // TODO
    if (c->capacity == c->size)
    {
      c = resize_cluster(c, c->capacity+CLUSTER_CHUNK);
    }

    c->obj[c->size] = obj;
    (c->size)++;
}

/*
 Seradi objekty ve shluku 'c' vzestupne podle jejich identifikacniho cisla.
 */
void sort_cluster(struct cluster_t *c);

/*
 Do shluku 'c1' prida objekty 'c2'. Shluk 'c1' bude v pripade nutnosti rozsiren.
 Objekty ve shluku 'c1' budou serazny vzestupne podle identifikacniho cisla.
 Shluk 'c2' bude nezmenen.
 */
void merge_clusters(struct cluster_t *c1, struct cluster_t *c2)
{
    assert(c1 != NULL);
    assert(c2 != NULL);

    // TODO

    for(int i = 0; i < c2->size; i++)
    {
      append_cluster(c1, c2->obj[i]);
    }

    sort_cluster(c1);
}

/**********************************************************************/
/* Prace s polem shluku */

/*
 Odstrani shluk z pole shluku 'carr'. Pole shluku obsahuje 'narr' polozek
 (shluku). Shluk pro odstraneni se nachazi na indexu 'idx'. Funkce vraci novy
 pocet shluku v poli.
*/
int remove_cluster(struct cluster_t *carr, int narr, int idx)
{
    assert(idx < narr);
    assert(narr > 0);

    // TODO

    clear_cluster(carr+idx);
    if (idx != narr-1)
    {
      init_cluster(carr+idx, (carr+(narr-1))->size);
      merge_clusters(carr+idx, carr+(narr-1));
			clear_cluster(carr+(narr-1));
    }

    return narr-1;
}

/*
 Pocita Euklidovskou vzdalenost mezi dvema objekty.
 */
float obj_distance(struct obj_t *o1, struct obj_t *o2)
{
    assert(o1 != NULL);
    assert(o2 != NULL);

    // TODO

    return (sqrtf(  (o1->x-o2->x)*(o1->x-o2->x) + (o1->y-o2->y)*(o1->y-o2->y) ));
}

/*
 Pocita vzdalenost dvou shluku.
*/
float cluster_distance(struct cluster_t *c1, struct cluster_t *c2)
{
    assert(c1 != NULL);
    assert(c1->size > 0);
    assert(c2 != NULL);
    assert(c2->size > 0);

    // TODO

    float f_maxDistance = 0,f_pom;

    for (int i = 0; i < c1->size; i++)
    {
      for (int j = 0; j < c2->size; j++)
      {
        f_pom = obj_distance(c1->obj+i,c2->obj+j);
        if ( f_pom > f_maxDistance)
        {
          f_maxDistance = f_pom;
        }
      }
    }
    return f_maxDistance;
}

/*
 Funkce najde dva nejblizsi shluky. V poli shluku 'carr' o velikosti 'narr'
 hleda dva nejblizsi shluky. Nalezene shluky identifikuje jejich indexy v poli
 'carr'. Funkce nalezene shluky (indexy do pole 'carr') uklada do pameti na
 adresu 'c1' resp. 'c2'.
*/
void find_neighbours(struct cluster_t *carr, int narr, int *c1, int *c2)
{
    assert(narr > 0);

    // TODO

    float f_minDistance = cluster_distance(carr,carr+1);
    float f_pom;

    for (int i = 0; i < narr - 1; i++)
    {
      for (int j = i+1; j < narr; j++)
      {
        if (i != j)
        {
          f_pom = cluster_distance(carr+i, carr+j);
          if (f_pom <= f_minDistance)
          {
            f_minDistance = f_pom;
            *c1 = i;
            *c2 = j;
          }
        }
      }
    }
}

// pomocna funkce pro razeni shluku
static int obj_sort_compar(const void *a, const void *b)
{
    // TUTO FUNKCI NEMENTE

    const struct obj_t *o1 = a;
    const struct obj_t *o2 = b;
    if (o1->id < o2->id) return -1;
    if (o1->id > o2->id) return 1;
    return 0;
}

/*
 Razeni objektu ve shluku vzestupne podle jejich identifikatoru.
*/
void sort_cluster(struct cluster_t *c)
{
    // TUTO FUNKCI NEMENTE

    qsort(c->obj, c->size, sizeof(struct obj_t), &obj_sort_compar);
}

/*
 Tisk shluku 'c' na stdout.
*/

void print_cluster(struct cluster_t *c)
{
    // TUTO FUNKCI NEMENTE

    for (int i = 0; i < c->size; i++)
    {
        if (i) putchar(' ');
        printf("%d[%g,%g]", c->obj[i].id, c->obj[i].x, c->obj[i].y);
    }
    putchar('\n');
    //printf("hotovo\n" );
}

/*
 Ze souboru 'filename' nacte objekty. Pro kazdy objekt vytvori shluk a ulozi
 jej do pole shluku. Alokuje prostor pro pole vsech shluku a ukazatel na prvni
 polozku pole (ukalazatel na prvni shluk v alokovanem poli) ulozi do pameti,
 kam se odkazuje parametr 'arr'. Funkce vraci pocet nactenych objektu (shluku).
 V pripade nejake chyby uklada do pameti, kam se odkazuje 'arr', hodnotu NULL.
*/
int load_clusters(char *filename, struct cluster_t **arr)
{
    assert(arr != NULL);

    // TODO

    int i=0,j,k,count;

    FILE* my_File = fopen(filename, "r");

    if (my_File == NULL)
    {
      arr = NULL;
      return i;
    }

    struct obj_t myObject;

    if (fscanf(my_File, "count=%d\n", &count) != 1)
    {
      return i;
      arr = NULL;
    }

    *arr = malloc(count * sizeof(struct cluster_t));

    assert (*arr != NULL);

    for(; i<count; i++)
    {
      if (fscanf(my_File, "%d %f %f\n", &myObject.id, &myObject.x, &myObject.y)==3 && myObject.x >= 0 && myObject.x <= 1000 && myObject.y >=0 && myObject.y <= 1000)
      {
        init_cluster(*arr+i, 1);
        *((*arr+i)->obj) = myObject;
        (*arr+i)->size = 1;

        for (j = 0; j < i;j++)
        {
          if ((*arr+j)->obj->id == myObject.id)
          {
            for (k = 0; k < i+1;k++)
            {
              clear_cluster (*arr+k);
            }

            free(*arr);
            *arr = NULL;
            fclose(my_File);
            return -2;
          }
        }
      }
      else
      {
        for ( k = 0; k < i;k++)
        {
          clear_cluster (*arr+k);
        }
        free(*arr);
        *arr = NULL;
        fclose(my_File);
        return -1;
      }
    }

    if (i < count || fscanf(my_File, "%d %f %f\n", &myObject.id, &myObject.x, &myObject.y)>0)
    {
      for (k = 0; k < i;k++)
      {
        clear_cluster (*arr+k);
      }

      free(*arr);
      *arr = NULL;
      fclose(my_File);
      return -1;
    }

		fclose(my_File);

    return i;
}

/*
 Tisk pole shluku. Parametr 'carr' je ukazatel na prvni polozku (shluk).
 Tiskne se prvnich 'narr' shluku.
*/
void print_clusters(struct cluster_t *carr, int narr)
{
    printf("Clusters:\n");
    for (int i = 0; i < narr; i++)
    {
        printf("cluster %d: ", i);
        print_cluster(&carr[i]);
    }
}

int main(int argc, char *argv[])
{
    struct cluster_t *clusters;

    // TODO

    long l_asked = 1, l_ackual;
    char* ptr;

    switch (argc)
    {
      case 2:
      break;
      case 3:
      {
        l_asked = strtol(argv[2], &ptr, 10);
        if (*ptr != '\0' || l_asked < 1 || strtol(argv[2], &ptr, 10) > INT_MAX)
        {
          fprintf(stderr,"Program ukonceny, bol zadany nespravny 2.argument!\n1. argument-nazov suboru\n2.argument(volitelny)-vysledny pocet zhlukov(INTEGER>0).\n");
          return 1;
        }
      }
      break;
      default:
      {
        fprintf(stderr, "Program ukonceny, neocakavane argumenty!\n1. argument-nazov suboru\n2.argument(volitelny)-vysledny pocet zhlukov>0.\n");
        return 1;
      }
    }

    l_ackual = load_clusters(argv[1], &clusters);

    if (l_ackual == -2)
    {
      fprintf(stderr, "Program ukonceny, viac objektov s rovnakym ID.\n");
      return 1;
    }

    if (l_ackual == -1)
    {
      fprintf(stderr, "Program ukonceny, nespravny format suboru.\n");
      return 1;
    }

    if (l_ackual == 0)
    {
      fprintf(stderr, "Program ukonceny, neboli nacitane ziadne zhluky zo suboru.\n");
      return 1;
    }

    if(l_ackual < l_asked)
    {
      for (unsigned int i = 0; i < l_ackual; i++)
      {
        clear_cluster(clusters+i);
      }

      free(clusters);
      fprintf(stderr, "Program ukonceny, pozadovany vysledny pocet zhlukov vacsi ako pocet zhlukov na vstupe.\n");
      return 1;
    }

    int a = -1, b = -1;

    while (l_ackual > l_asked)
    {
      find_neighbours (clusters, l_ackual, &a, &b);
      merge_clusters (clusters+a, clusters+b);
      l_ackual = remove_cluster (clusters, l_ackual, b);
    }

    print_clusters (clusters, l_ackual);

    for (unsigned int i = 0; i < l_ackual; i++)
    {
      clear_cluster(clusters+i);
    }

    free(clusters);

    return 0;
}
