#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "malloc.h"
#include <unistd.h>

#include "spm/utils.h"
#include "spm/libspm.h"
#include "ecmp.h"





#ifdef STATIC
int open_ecmp(char* path,struct package* pkg)
#else
int open(char* path,struct package* pkg)
#endif
{
    struct ecmp_section** sections = calloc(32,sizeof(struct ecmp_section*));
    printf("sections is %p\n",sections);

    // get sections from file
    // WARNING: this function allocates memory for sections
    printf("Parsing file %s\n",path);
    unsigned int section_count = ecmp_parse_file(&sections,path);
    printf("file is parsed , launching section analysis\n");
    printf("sections address is %p and value is %p\n",&sections,sections);

    // parse sections
    for (int i = 1; i < section_count; i++)
    {   
        printf("section is : %s\n",sections[i]->name);
        if (strcmp(sections[i]->name,"info") == 0)
        {
            printf("parsing info section\n");
            // parse info section
            
            // print buff
            printf("buff is \n%s\n",sections[i]->buff);
            char* lines[64];
            int line_count = split(sections[i]->buff,'\n',lines);
            printf("line count is %d and lines is %p\n",line_count,lines);
            for (int j = 0; j < line_count-2; j++)
            {
                printf("run %d\n",j);
                printf("line is : %s\n",lines[j]);
                char* key = strtok(lines[j],"=");
                char* value = strtok(NULL,"=");
                printf("key is %s and value is %s\n",key,value);


                if (strcmp(key,"name") == 0)
                {
                    pkg->name = value;
                }
                else if (strcmp(key,"type") == 0)
                {
                    pkg->type = value;
                }
                else if (strcmp(key,"version") == 0)
                {
                    pkg->version = value;
                }
                else if (strcmp(key,"license") == 0)
                {
                    printf("license is %s\n",value);
                    pkg->license = value;
                    printf("license is %s\n",pkg->license);
                }
                else if (strcmp(key,"url") == 0)
                {
                    printf("url is %s\n",value);
                    pkg->url = value;
                }
                else
                {
                    printf("Unknown info key: %s\n",key);
                }
            }

            printf("info section is parsed\n");
                
        }
        else if (strcmp(sections[i]->name,"makedeps") == 0)
        {
           pkg->makedependenciesCount = split(sections[i]->buff,'\n',pkg->makedependencies);

            
        }
        else if (strcmp(sections[i]->name,"dependencies") == 0)
        {
            // parse dependencies section
            pkg->dependenciesCount = split(sections[i]->buff,'\n',pkg->dependencies);
        }
        else if (strcmp(sections[i]->name,"locations") == 0)
        {
            // parse scripts section
            pkg->locationsCount = split(sections[i]->buff,'\n',pkg->locations);
        }
        else if (strcmp(sections[i]->name,"install") == 0)
        {
            // parse scripts section
            pkg->info.install = sections[i]->buff;
        }
        else if (strcmp(sections[i]->name,"download") == 0)
        {
            // parse scripts section
            pkg->info.download = sections[i]->buff;
        }
        else if (strcmp(sections[i]->name,"special") == 0)
        {
            // parse scripts section
            pkg->info.special = sections[i]->buff;
        }
        else {
            // unknown section
            printf("Unknown section: %s\n",sections[i]->name);
        }

    }
    return 0;

}

#ifdef STATIC
int create_ecmp(char* path,struct package* pkg)
#else
int create(char* path,struct package* pkg)
#endif
{
    msg(DBG3,"Creating ecmp file %s",path);
    // create file
    FILE* file = fopen(path,"w");
    if (file == NULL)
    {
        printf("Error creating file %s\n",path);
        return 1;
    }

    msg(DBG3,"Writing info section");
    // write info section
    fprintf(file,"[info]\n");
    msg(DBG3,"Writing name");
    if (pkg->name != NULL) fprintf(file,"name=%s\n",pkg->name);

    msg(DBG3,"Writing version");
    if (pkg->version != NULL) fprintf(file,"version=%s\n",pkg->version);

    msg(DBG3,"Writing type");
    if (pkg->type != NULL) fprintf(file,"type=%s\n",pkg->type);

    msg(DBG3,"Writing license");
    if (pkg->license != NULL) fprintf(file,"license=%s\n",pkg->license);

    msg(DBG3,"Writing url");
    if (pkg->url != NULL) fprintf(file,"url=%s\n",pkg->url);


    msg(DBG3,"Writing makedeps section : %s",pkg->makedependencies);
    if (pkg->makedependencies != NULL)
    {
        fprintf(file,"\n[makedeps]\n");
        for (int i = 0; i < pkg->makedependenciesCount; i++)
        {
            fprintf(file,"%s\n",pkg->makedependencies[i]);
        }
    }

    msg(DBG3,"Writing dependencies section");
    fprintf(file,"\n[dependencies]\n");
    for (int i = 0; i < pkg->dependenciesCount; i++)
    {
        fprintf(file,"%s\n",pkg->dependencies[i]);
    }

    if (pkg->info.download != NULL) {
        fprintf(file,"[download]\n");
        fprintf(file, "%s\n",pkg->info.download);
    }

    msg(DBG3,"Writing install section");
    fprintf(file,"\n[install]\n");
    // compatibility
    if (pkg->info.prepare != NULL) fprintf(file,"%s\n",pkg->info.prepare);
    if (pkg->info.make != NULL) fprintf(file,"%s\n",pkg->info.make);
    fprintf(file,"%s\n",pkg->info.install);

    


    
    msg(DBG3,"Writing special section : %s",pkg->info.special);
    if (pkg->info.special != NULL)
    {
        fprintf(file,"\n[special]\n");
        fprintf(file,"%s\n",pkg->info.special);
    }

    msg(DBG3,"Writing locations section : %s",pkg->info.special);
    if (pkg->locations != NULL)
    {
        fprintf(file,"\n[locations]\n");
        for (int i = 0; i < pkg->locationsCount; i++)
        {
            fprintf(file,"%s\n",pkg->locations[i]);
        }
    }
    fclose(file);
    msg(INFO,"Created ecmp file %s",path);


    return 0;
}


unsigned int ecmp_parse_file(struct ecmp_section*** sections,char* path)
{   
    printf("sections is %p and size if %lu\n",*sections,malloc_usable_size(*sections));

    bool RM_SPACE = false;

    // check if file exists
    if (access(path,F_OK) != 0)
    {
        printf("File %s does not exist\n",path);
        return 0;
    }

    // I had a small problem th t cause the section lis to start at 1 , so this solution is tupid
    int sec_count = 0;
    

    // read file line by line
    FILE* fp = fopen(path,"r");
    char* line = NULL;
    size_t len = 0;
    ssize_t read;

    printf("Reading file %s\n",path);
    while((read = getline(&line,&len,fp)) != -1)
    {

        // remove spaces from line
        if (RM_SPACE)
        {
            for (int i = 0; i < strlen(line); i++)
            {
                if (line[i] == ' ')
                {
                    popcharn(line, strlen(line), i);
                }
            }
            printf("space removed: %s",line);
        }
        

        printf("Line: %s with %lu chars\n",line,strlen(line));


        // remove empty line
        if (line[0] == '\n' ) {
            printf("Empty line\n");
            continue;
        }

        // ignore comments
        if(line[0] == '#') continue;
        
        
        if (line[0] == '[') {
            RM_SPACE = false;
            sec_count++;
            // allocate new section
            (*sections)[sec_count] = calloc(1,sizeof(struct ecmp_section));
            // get section name
            (*sections)[sec_count]->name = calloc(strlen(line),sizeof(char));

            // section name
            printf("parsing section header\n");
            for (int i = 1; i < strlen(line); i++) {
                
                if (line[i] == ']') {
                    (*sections)[sec_count]->name[i] = '\0';
                    break;
                }
                else {
                    (*sections)[sec_count]->name[i-1] = line[i];
                    printf("char: %c\n",line[i]); 
                }
            }
            printf("section name: %s\n",(*sections)[sec_count]->name);
            // allocate section buffer
            (*sections)[sec_count]->buff = calloc(512, sizeof(char));
            
            if ((strcmp((*sections)[sec_count]->name,"info") == 0) ||
            (strcmp((*sections)[sec_count]->name,"makedeps" ) == 0) ||
            (strcmp((*sections)[sec_count]->name,"dependencies") == 0) ||
            (strcmp((*sections)[sec_count]->name,"locations") == 0))
            {
                RM_SPACE = true;
            }

            continue;
        }

        // check if buff is null
        if (sec_count == 0) {
            continue;
        }

        add_line:

        // append line to buffer
        // check if buffer is full
        printf("buff size: %lu, line is %s , appending\n",strlen((*sections)[sec_count]->buff),line);
        if ((strlen((*sections)[sec_count]->buff) + strlen(line)) * sizeof(char) > malloc_usable_size((*sections)[sec_count]->buff)) {
            printf("buffer full");
            // reallocate buffer
            (*sections)[sec_count]->buff = realloc((*sections)[sec_count]->buff, malloc_usable_size((*sections)[sec_count]->buff) + 512);
            goto add_line;
        }
        strcat((*sections)[sec_count]->buff,line);
        printf("buff: %s\n",(*sections)[sec_count]->buff);
    }

    fclose(fp);
    printf("sections address is %p and value is %p\n",sections,*sections);
    
    printf("sec_coutn is %d\n",sec_count);
    return sec_count;
}