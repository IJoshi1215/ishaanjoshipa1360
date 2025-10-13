#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <getopt.h>

typedef struct 
{
   int valid;
   unsigned long tag;
   unsigned long Iru;
} Cacheline;

typedef struct 
{
   Cacheline *lines;
} CacheSet;

typedef struct
{
  CacheSet *sets;
  int s;
  int E;
  int b;
  unsigned long time;
} Cache;

static int hit_count = 0;
static int miss_count = 0;
static int eviction_count = 0;


Cache * init_cache(int s, int E, int b)
{
   Cache *cache = malloc(sizeof(Cache));

   if(!cache)
   {
      perror("Error");
      exit(1);
   }

   cache->s = s;
   cache->E = E;
   cache->b = b;
   cache-> time = 0;

   int S = 1 << s;

   cache-> sets = malloc(S *sizeof(CacheSet));
   if(!cache-> sets)
   {
      perror("malloc sets");
      free(cache);
      exit(1);
   }

   for(int i = 0; i < S; i++)
   {
      if(!cache-> sets[i].lines) 
      {
         perror("malloc lines");

         for(int j = 0; j < i; j++) free(cache->sets[j].lines);
         free(cache->sets);
         free(cache);
         exit(1);

      }

      for(int j = 0; j < E; j++)
      {
         cache->sets[i].lines[j].valid = 0;
         cache->sets[i].lines[j].tag = 0;
         cache->sets[i].lines[j].Iru = 0;
      }
   }
    return cache;
}


void free_cache(Cache * cache)
{
   if(!cache) return;
   int S = 1 << cache->s;

   for(int i = 0; i < S; i++)
   {
      free(cache->sets[i].lines);

   }

   free(cache->sets);
   free(cache);
}

int access_cache(Cache * cache, unsigned addr, int * evicted)
{
   cache-> time++;
   int s = cache ->s;
   int b = cache->b;
   int E = cache->E;

   unsigned long set_index = (addr >> b) & ((1UL << s) -1);
   unsigned long tag = addr >> (s+b);

   CacheSet *set  = &cache->sets[set_index];


   int hit_index = -1;

   for(int i = 0; i < E; i++)
   {
      if(set->lines[i].valid && set->lines[i].tag == tag)
      {
         hit_index = i;
         break;
      }
   }

   if(hit_index != -1)
   {
     set->lines[hit_index].Iru = cache->time;
     hit_count++;
     if(evicted) * evicted = 0;
     return 1;
   }

   miss_count++;

   int empty_index = -1;

   for(int i = 0; i < E; i++)
   {
      if(set->lines[i].valid)
      {
         empty_index = i;
         break;
      }
   }  

   if(empty_index != -1)
   {
      set->lines[empty_index].valid = 1;
      set->lines[empty_index].tag = tag;
      set->lines[empty_index].Iru = cache->time;
      if(evicted) * evicted = 0;
      return 0;

   }

   int Iru_index = -1;

   unsigned long min_Iru = set->lines[0].Iru;

   for(int i =0; i < E; i++)
   {
      if(set->lines[i].Iru < min_Iru)
      {
         min_Iru = set->lines[i].Iru;
         Iru_index= i;
      }
   }

   set->lines[Iru_index].tag = tag;
   set->lines[Iru_index].Iru = cache->time;
   eviction_count++;
   if(evicted) * evicted = 0;
   return 0;
}

void process_op(Cache *cache, char op, unsigned long addr, int verbose)
{
   if(op == 'I')
   {
      return;
   }

   if(op == 'L' || op == 'S')
   {
      int ev = 0;
      int was_hit = access_cache(cache,addr,&ev);
      if(verbose)
      {
         if(was_hit)
         {
            printf("%c %lx,1hit\n",op,addr);
         }
         else
         {
            if(ev)
            {
               printf("%c %lx,1 miss eviction\n",op,addr);

            }
            else
            {
               printf("%c %lx,1miss\n", op,addr);
            }
         }
      }
   }

   else if(op == 'M')
   {
      int ev = 0;
      int was_hit = access_cache(cache,addr,&ev);

      if(verbose)
      {
         if(was_hit)
         {
            printf("M %lx,1hit",addr);

         }
         else 
         {
            if(ev)
            {
               printf("M %lx,1 miss eviction",addr);
               printf("M %lx, 1 miss",addr);
            }
         }
      }
   }

   int ev2 = 0;
   int was_hit2 = access_cache(cache,addr,&ev2);
   if(verbose)
   {
      if(was_hit2)
      {
         printf("hit\n");

      }
      else
      {
         if(ev2)
         {
            printf("miss eviction\n");
         }
         else
         {
            printf("miss\n");
         }
      }
   }

}

void print_usage(char *argv[])
{
   printf(" Usage: %s[-hv] -s <s> -E <E> -b <b> -t <tracefile>\n",argv[0]);
   printf(" -h  print this to help message\n");
   printf(" -v verbose output\n");
   printf(" -s <s> number of set of index bits\n");
   printf(" -E <E> number of lines per set\n");
   printf(" -b<b> number of block bits\n");
   printf("-t <file> trace file\n");
}

void print_summary(int hit_count,int miss_count,int eviction_count)
{
   printf("%d,%d,%d\n",hit_count,miss_count,eviction_count);
}



int main(int argc,char *argv[])
{
    
    int s = 0, E = 0, b = 0; 
    char *traceFile = NULL; 
    int opt;
    int verbose = 0;

    while ((opt = getopt(argc,argv,"s:E:b:t:vh"))!= -1)
    {
      switch(opt)
      {
         case 's':
         s = atoi(optarg); 
         break;

         case 'E':
         E = atoi(optarg);
         break;

         case 'b':
         b = atoi(optarg);
         break;

         case 't':
         traceFile = optarg; 
         break;

         case 'v':
         verbose = 1;
         break;

         case 'h':
         printf("Usage: ./cachesim -s <s> -E <E> -b <b> -t <tracefile>\n");
         exit(0);

         default:
         fprintf(stderr,"Invalid option\n");
         exit(1);
      }
    }

    if(s < 0 || E < 0 || b < 0 || !traceFile)
    {
      fprintf(stderr,"Invalid arguments\n");
      return 1;
    }

    Cache *cache = init_cache(s,E,b);
    FILE *fp = fopen(traceFile,"r");

    if(!fp)
    {
      fprintf(stderr,"Error opening file %s: %d\n",traceFile,errno);
      free_cache(cache);
      return 1;
    }

    char line[256];

    while(fgets(line,sizeof(line),fp))
    {
      char op;
      unsigned long addr;
      int size;
      if(sscanf(line, "%c %lx,%d", &op,&addr,&size)!= 3)
      {
         continue;
      }
      if(op == 'I')
      {
         continue;
      }

      process_op(cache,op,addr,verbose);
    }
    fclose(fp);

    print_summary(hit_count,miss_count,eviction_count);

    free_cache(cache);
    return 0;
} 




