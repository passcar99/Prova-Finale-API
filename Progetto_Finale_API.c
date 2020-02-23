#include <stdio.h>
#include <stdlib.h>
#define SIZE 60
#define MAXSTRING 188
#define BLACK 0
#define RED 1

typedef struct entity{
    char *name;
    struct entity * left;
    struct entity * right;
    struct entity * parent;
    struct entity * connection_tree[3];
    int dim;
    unsigned int color : 1;
}entity;

typedef struct ent_list{
    char *name;
    struct ent_list * next;
}ent_list;

typedef struct to_del_ent{
    entity * ent;
    struct to_del_ent * next;
}to_del_ent;

typedef struct rel_list{
    char name[SIZE];
    int max;  // dimensione sottoalbero connessioni massimo
    ent_list * max_p; // puntatore a lista elementi con numero massimo di connessioni
    struct rel_list *next;
    struct rel_list *prev;
    entity * rel_tree;
}rel_list;

rel_list * relations = NULL;
entity * entity_tree[23][23] = {NULL}; //matrice puntatori a alberi entità esistenti

//command functions
void report();
void addent(char *name);
void delent(char *name);
void addrel(char *id_orig, char *id_dest, char *id_rel);
void delrel(char *id_orig, char *id_dest, char *id_rel);


//tree functions

entity * new_entity(char *name, entity **entity_root, rel_list * relation, entity * dest);
entity * entity_successor(entity *entity_root, entity * punt);
entity *search_entity(char *name, entity * entity_root, short int max);
void delete_entity_from_rel(char *name);
void delete_entity(char *name, entity **entity_root, rel_list * relation, entity * dest);
void delete_from_connection_tree(char * name, entity *p, rel_list *relation);
void delete_type_of_relation(char *name, rel_list * relation);
rel_list * insert_search_type_of_relation(char *name, rel_list ** list, int flag);
void create_list(entity *entity_root, rel_list * relation);
void delete_from_list (ent_list ** entities_list, char *name);
void delete_list(ent_list ** entities_list);
to_del_ent * to_delete_list = NULL;
void delete_tree(entity **entity_root);
void left_rotate_rb(entity ** entity_root, entity * to_rotate);
void right_rotate_rb(entity ** entity_root, entity * to_rotate);
void rb_insert_fixup(entity ** entity_root, entity * to_fix);
void rb_delete_fixup(entity ** entity_root, entity * to_fix);

//utility functions
void sscanf2(char *string, char *command, char * id_orig, char *id_dest, char *id_rel);
void print_numero(int num, char *string);
int strcmp(char *s1, char *s2);
void pstrcpy(char *s1, char *s2);
int pstrlen(char * s1);
/*Progetto finale API. Le entità sono memorizzate in alberi disposti su una matrice 23x23 
assegnati con un una funzione di hashing molto semplice: metodo della divisione sulla prima 
lettera significativa dell'identificativo (in posizione 1 partendo da 0) per selazionare 
la riga e sulla seconda lettera significativa(in posizione 2 partendo da 0) per selezionare 
la colonna. Le relazioni, essendo "poche", sono memorizzate con una lista ordinata, cosa che 
facilita report() e ogni relazione ha come attributi la dimensione del massimo numero di
relazioni entranti e la lista ordinata dei puntatori ai nomi che soddisfano tale massimo.
Le connessioni sono rappresentate con un albero delle entità riceventi, le entità di partenza 
sono memorizzate in tre alberi stabiliti con un hashing basilare sulla prima lettera 
significativa del nome. Ogni entità ricevente ha un campo dim che indica la dimensione del-
l'albero delle entità di partenza.*/
//*************************************************************//
int main(){
    char string[MAXSTRING];
    char command[7];
    char id_orig[SIZE], id_dest[SIZE], id_rel[SIZE];
    while(1){
        sscanf2(string, command, id_orig, id_dest, id_rel);
        switch(command[0]){
            case 'e':  
                return 0;   
            case 'r':
                report();
                break;
            case 'a':
                if(command[3]=='e'){
                    addent(id_orig);
                }
                else{
                    addrel(id_orig, id_dest, id_rel);
                }
                break;
            case 'd':
                if(command[3]=='e'){
                    delent(id_orig);
                }
                else{
                    delrel(id_orig, id_dest, id_rel);
                }
                break;
        }
    }
    return 0;
}

/*command functions*/

void addrel(char *id_orig, char *id_dest, char *id_rel){
    entity * new;
    if(search_entity(id_dest, entity_tree[id_dest[1]%23][id_dest[2]%23], 0)==NULL || search_entity(id_orig, entity_tree[id_orig[1]%23][id_orig[2]%23], 0)==NULL)
        return;
    rel_list * relation = insert_search_type_of_relation(id_rel, &relations, 1);
    new = new_entity(id_dest, &(relation->rel_tree), relation, NULL);
    new_entity(id_orig, &((new->connection_tree)[id_orig[1]%3]), relation, new);
}

void delrel(char *id_orig, char *id_dest, char *id_rel){
    rel_list * relation = insert_search_type_of_relation(id_rel, &relations, 0);
    entity * dest;
    if(relation == NULL) return;
    dest = search_entity(id_dest, relation->rel_tree, 0);
    if(dest == NULL) return;
    delete_entity(id_orig, &((dest->connection_tree)[id_orig[1]%3]), relation, dest);
    if(relation->rel_tree == NULL || relation -> max == 0) //posso evitare una condizione
        delete_type_of_relation(id_rel, relation);
}

void addent(char *name){
    new_entity(name, &entity_tree[name[1]%23][name[2]%23], NULL, NULL);
}

void delent(char * name){
    delete_entity_from_rel(name); 
    delete_entity(name, &entity_tree[name[1]%23][name[2]%23], NULL, NULL);
}

void report (){
    rel_list * p = relations;
    ent_list *max_ent=NULL;
    char numero[20];
    int flag = 0;
    if (p == NULL){
        puts("none");
        return;
    }
    while(p != NULL){
        if(p->max != 0){
            fputs(p->name, stdout);
            putc(' ', stdout);
            max_ent = p -> max_p;
            while(max_ent != NULL){
                fputs(max_ent -> name, stdout); 
                putc(' ', stdout);
                max_ent = max_ent -> next;
            }
            print_numero(p->max, numero);
            fputs("; ", stdout);
            flag = 1;
        }
        p = p -> next;
    }
    if(flag == 0)
        fputs("none", stdout);
    putchar('\n');
}

/*tree functions */

entity * new_entity(char *name, entity ** entity_root, rel_list * relation, entity * dest){
    entity *p = (*entity_root), *new, *q, *ent; //ent è il puntatore all'entità con quel nome nell'albero delle entità
    ent_list *prec, *punt, *elem;
    int diff = -1;
    ent = search_entity(name, entity_tree[name[1]%23][name[2]%23], 0);//vedere come efficientare
    if(ent != NULL && relation == NULL && dest == NULL) 
        return ent;
    if (p == NULL){   //per albero relazioni
        p = (entity*) malloc(sizeof(entity));
        if(ent == NULL && relation == NULL && dest == NULL){
            p -> name = (char*) malloc(pstrlen(name)+1);
            pstrcpy(p -> name, name);
        }
        else
            p->name = ent -> name;
        p->left = NULL;
        p->right = NULL;
        p->parent = NULL;
        for(int i =0; i < 3; i++)
            (p->connection_tree)[i] = NULL;
        (*entity_root) = p;
        p -> color = BLACK;
        p-> dim = 0;
        if(relation != NULL){
            if(dest != NULL){
                dest ->dim ++;
                if(dest -> dim > relation -> max){
                delete_list(&(relation -> max_p));
                relation -> max = dest -> dim;
                relation -> max_p = (ent_list*) malloc(sizeof(ent_list));
                relation -> max_p -> next = NULL;
                relation -> max_p -> name = dest -> name;     
                }
                else 
                if(dest->dim == relation -> max){
                    for(punt = relation->max_p, prec = NULL; punt != NULL && strcmp(punt->name, dest->name)<0; punt = punt->next){
                        prec = punt; 
                    }
                    elem = (ent_list*) malloc(sizeof(ent_list));
                    elem->name = dest->name;
                    elem->next = relation -> max_p;
                    if(prec == NULL){
                        relation->max_p = elem;
                    }
                    else{
                        prec->next = elem;
                        elem->next = punt;
                    } 
                
                }
                
            }
        }
        return p;
    }
    while(p!=NULL && (diff = strcmp(name, p -> name))!= 0){
        q = p;
        if (diff<0) {
                    p = p->left;
                    }
        if (diff>0) {
                    p=p->right;
                    }
    }
    if (diff != 0){
        new = (entity *) malloc(sizeof(entity));
        if(ent == NULL && relation == NULL && dest == NULL){
            new -> name = (char*) malloc(pstrlen(name)+1);
            pstrcpy(new -> name, name);
        }
        else
            new -> name = ent -> name;
        new->parent= q;
        if(q!=NULL){
            if(diff<0) q->left=new;
                else q->right = new;
        }
        for(int i =0; i < 3; i++)
            (new->connection_tree)[i] = NULL;
        new -> left = NULL;
        new -> right = NULL;
        new -> color = RED;
        new -> dim = 0;
        if(relation != NULL){
            if(dest != NULL){
                dest->dim ++;
                if(dest -> dim > relation -> max){
                    relation -> max = dest -> dim;
                    delete_list(&(relation->max_p));
                    relation -> max_p = (ent_list*) malloc(sizeof(ent_list));
                    relation -> max_p -> next = NULL;
                    relation -> max_p -> name = dest-> name;
                }
                else 
                if(dest->dim == relation -> max){
                    for(punt = relation->max_p, prec = NULL; punt != NULL && strcmp(punt->name, dest->name)<0; punt = punt->next){
                        prec = punt; 
                    }
                    elem = (ent_list*) malloc(sizeof(ent_list));
                    elem->name = dest->name;
                    elem->next = relation -> max_p;
                    if(prec == NULL){
                        relation->max_p = elem;
                    }
                    else{
                        prec->next = elem;
                        elem->next = punt;
                    } 
                
                }
            }
        }
        rb_insert_fixup(entity_root, new);
        return new;
    }
    return p;
}

entity * entity_successor(entity *entity_root, entity * punt){
    entity *q;
    if (punt == NULL) return NULL;
    if(punt->right != NULL ){
        return search_entity(punt->name, punt->right, -1);
    }
    q = punt->parent;
    while(q != NULL && punt == q->right){
        punt = q;
        q = q->parent; //funziona se abbinata a delete_entity (non funziona con foglia estrema destra)
    }
    return q;
}

entity *search_entity(char *name, entity *entity_root,short int max){ // 0 per cercare un elemento passato per chiave, -1 per cercare il minimo
    entity *p = entity_root;
    int diff = -1; 
    if(p == NULL) return NULL;
    if(max == 0){
        while (p != NULL && diff != 0){
            diff=strcmp(name, p->name);
            if(diff<0)
                p=p->left;
            if(diff>0) 
                p=p->right;
        }
    }
    else
        while (p->left!=NULL){
            p = p->left;
        }
    return p;
}

void delete_entity(char *name, entity **entity_root, rel_list * relation, entity *dest){
    entity * p = NULL, *q = NULL, *to_delete = NULL, **entity_tree_delete = NULL;
    int direct_delete_flag = 0;
    if(name != NULL)
        to_delete = search_entity(name, *entity_root,0);
    else 
        to_delete = dest;
    if(to_delete == NULL)
        return;     
    if(dest != NULL && name != NULL){
        (dest) -> dim --;
        if(relation != NULL && relation -> max == (dest)-> dim + 1){
            delete_from_list(&(relation->max_p), (dest)->name);
            if(relation -> max_p == NULL){
                relation -> max = 0;
                delete_list(&(relation -> max_p));
                create_list(relation->rel_tree, relation);
            }
        }
        if((dest)->dim <= 0 && relation != NULL){
            to_delete = (dest);
            direct_delete_flag = 1;
        }
    }
    for(int i = 0; i < 3 ; i++)
        delete_tree(&((to_delete->connection_tree)[i]));
    if(to_delete->left == NULL && to_delete -> right == NULL && to_delete == entity_tree[(to_delete->name)[1]%23][(to_delete->name)[2]%23]){
        entity_tree_delete =&entity_tree[(to_delete->name)[1]%23][(to_delete->name)[2]%23];
        free(to_delete->name);
        free(to_delete);
        (*entity_tree_delete) = NULL;
        return;
    }
    if(to_delete->left==NULL || to_delete->right ==NULL) 
        q = to_delete;
    else
        if(direct_delete_flag)
            q = entity_successor(relation->rel_tree, to_delete);
        else
            q = entity_successor(*entity_root, to_delete);
    if(q == NULL)
        return;
    if(q->left != NULL) 
        p = q->left;
    else 
        p = q->right; 
    if(p != NULL)
        p->parent = q->parent;
    if (q->parent == NULL){
        if(direct_delete_flag)
            relation->rel_tree = p;
        else
            *entity_root = p;
    }  
    else if(q == q->parent->left) 
        q->parent->left = p;
    else 
        q->parent->right = p;
    if(*entity_root == entity_tree[(to_delete->name)[1]%23][(to_delete->name)[2]%23]){
        free(to_delete->name);
        to_delete->name = NULL;
    }
    if(q != to_delete){
        to_delete -> name = q -> name;
        for(int i = 0; i < 3 ; i ++)
            (to_delete->connection_tree)[i] = (q->connection_tree)[i];
        to_delete ->dim = q -> dim;
    }
    if(q->color == BLACK){
        if(direct_delete_flag)
            rb_delete_fixup(&(relation->rel_tree), p);
        else
            rb_delete_fixup(entity_root, p);
    }
    if(name != NULL)
       free(q); //memory leak con name == NULL, efficientare
}

void delete_entity_from_rel(char *name){
    rel_list* relation = relations, *p;
    to_del_ent * q;
    while(relation != NULL){
        delete_entity(name, &(relation->rel_tree), relation, NULL);
        delete_from_connection_tree(name, relation->rel_tree, relation);
        while(to_delete_list != NULL){
            delete_entity(NULL, &(relation->rel_tree), relation, to_delete_list ->ent);
            q = to_delete_list; 
            to_delete_list = to_delete_list->next;
            free(q);
        }
        relation -> max = 0;
        delete_list(&(relation -> max_p));
        create_list(relation -> rel_tree, relation);
        if(relation->rel_tree==NULL){
            p = relation->next;
            delete_type_of_relation(relation->name, relation);
            relation = p;
        }
        else
            relation = relation->next;
    }
}

void create_list(entity *entity_root, rel_list * relation){
    int dim = 0;
    ent_list * p , * q = NULL, *elem = NULL;
    if (entity_root == NULL) return;
    create_list(entity_root->left, relation);
    create_list(entity_root->right, relation); 
    dim = entity_root ->dim;
    if(relation -> max< dim){
        relation -> max = dim;
        delete_list(&(relation -> max_p));
        relation -> max_p = (ent_list*)malloc(sizeof(ent_list));
        relation -> max_p->name= entity_root->name;
        relation -> max_p->next = NULL;
        return;
    }
    else if(relation -> max == dim){
        for(p = relation ->max_p, q = NULL; p != NULL && strcmp(p->name, entity_root->name)<0; p = p->next){
            q = p; 
        }
        elem = (ent_list*) malloc(sizeof(ent_list));
        elem->name = entity_root->name;
        elem->next = relation->max_p;
        if(q == NULL){
            relation-> max_p = elem;
        }
        else{
            q->next = elem;
            elem->next = p;
        }
        return;
    }
}

void delete_from_list (ent_list ** entities_list, char *name){
    ent_list * punt, *prec, *elem;
    for(punt = (*entities_list), prec = NULL; punt != NULL && strcmp(punt->name, name)<0; punt = punt->next){
        prec = punt; 
        }
    if(prec == NULL){
        elem = punt;
        (*entities_list) = punt -> next;
        free(elem);
    }
    else{
        prec -> next = punt -> next;
        free(punt);
        } 
}

void delete_list(ent_list ** entities_list){ //v3 non dealloca memoria
    ent_list * p = *entities_list;
    while((*entities_list) != NULL){
        p = (*entities_list);
        (*entities_list)= (*entities_list)->next;
        free(p);
    }
}

void delete_tree(entity **entity_root){
    if(entity_root == NULL || *entity_root == NULL) 
        return;
    delete_tree(&((*entity_root)->left));
    delete_tree(&((*entity_root)->right));
    if((*entity_root)->left == NULL && (*entity_root)->right== NULL){
        for(int i = 0; i < 3; i ++)
            delete_tree(&(((*entity_root)->connection_tree)[i]));
        free(*entity_root);
        *entity_root = NULL;
        return;
    }
}



void delete_from_connection_tree(char * name, entity *p, rel_list * relation){
    to_del_ent *q = NULL;
    if(p == NULL) return;
    delete_from_connection_tree(name, p->left, relation);
    delete_from_connection_tree(name, p->right, relation);
    delete_entity(name, &((p->connection_tree)[name[1]%3]), NULL, p);
    if(p->dim == 0){
        q = (to_del_ent *) malloc (sizeof(to_del_ent));
        q -> ent = p;
        q -> next = to_delete_list;
        to_delete_list = q;
    }
}

void delete_type_of_relation(char *name, rel_list * relation){
    rel_list * p = (relation);
    if(p != NULL){
        if(p->next != NULL)
            p->next->prev = p->prev;
        if(p-> prev != NULL)
            p->prev->next = p->next;
        else
            relations = p -> next;
        delete_list(&(p->max_p));
        free(p);
        return;
    }
}

rel_list * insert_search_type_of_relation(char *name, rel_list ** list, int flag){ // flag == 0 per cercare, flag == 1 per inserire
    rel_list *p = (*list), *q = NULL, *elem;
    int diff = -1;
    while(p != NULL && (diff = strcmp(name, p -> name)) > 0){
        q = p;
        p = p -> next;
    }
    if(p == NULL && flag == 0) //esaurito lista e non lo ho trovato
        return NULL;
    if( diff == 0) return p;
    elem = (rel_list*) malloc(sizeof(rel_list));
    pstrcpy(elem->name, name);
    elem ->rel_tree = NULL;
    elem ->max_p = NULL;
    elem -> max = 0;
    elem ->next = p;
    if(p != NULL)
        p->prev = elem;
    if(q == NULL){
        (*list) = elem;
        elem -> prev = NULL;
    }
    else{
        q -> next = elem;
        elem -> prev = q;
    }
    return elem;
}

void left_rotate_rb(entity ** entity_root, entity * to_rotate){
    entity * q;
    if(to_rotate == NULL)
        return;
    q = to_rotate -> right;
    if(q == NULL)
        return;
    to_rotate -> right = q -> left;
    if(q -> left != NULL) 
        q -> left -> parent = to_rotate;
    q -> parent = to_rotate -> parent;
    if (to_rotate -> parent == NULL)
        (*entity_root) = q;
    else if(to_rotate == to_rotate -> parent -> left)
        to_rotate -> parent -> left = q;
    else
        to_rotate -> parent -> right = q;
    q -> left = to_rotate;
    to_rotate -> parent = q;  
}

void right_rotate_rb(entity ** entity_root, entity * to_rotate){
    entity * q;
    if(to_rotate == NULL)
        return;
    q = to_rotate -> left;
    if(q == NULL)
        return;
    to_rotate -> left = q -> right;
    if(q -> right != NULL) 
        q -> right -> parent = to_rotate;
    q -> parent = to_rotate -> parent;
    if (to_rotate -> parent == NULL)
        (*entity_root) = q;
    else if(to_rotate == to_rotate -> parent -> right)
        to_rotate -> parent -> right = q;
    else
        to_rotate -> parent -> left = q;
    q -> right = to_rotate;
    to_rotate -> parent = q;  
}

void rb_insert_fixup(entity ** entity_root, entity * to_fix){
    entity * p, *q; // su pseudocodice x == p, y == q
    if(to_fix == (*entity_root))
        (*entity_root)-> color = BLACK;
    else{
        p = to_fix -> parent;
        if(p != NULL && p -> parent != NULL){
        if(p->color == RED){
            if(p == p -> parent -> left){ //possibile segmentation error
                q = p -> parent -> right;//possible SE
                if(q != NULL && q -> color == RED){
                    p -> color = BLACK;
                    q -> color = BLACK;
                    p -> parent -> color = RED;
                    rb_insert_fixup(entity_root, p -> parent);
                }
                else if(to_fix == p -> right){
                    to_fix = p;
                    left_rotate_rb(entity_root, to_fix);
                    p = to_fix -> parent;
                }
                p -> color = BLACK;
                p -> parent -> color = RED;
                right_rotate_rb(entity_root, p ->parent);
            }
            else{
                q = p -> parent -> left;//possible SE
                if(q != NULL && q -> color == RED){
                    p -> color = BLACK;
                    q -> color = BLACK;
                    p -> parent -> color = RED;
                    rb_insert_fixup(entity_root, p -> parent);
                }
                else if(to_fix == p -> left){
                    to_fix = p;
                    right_rotate_rb(entity_root, to_fix);
                    p = to_fix -> parent;
                }
                p -> color = BLACK;
                p -> parent -> color = RED;
                left_rotate_rb(entity_root, p ->parent);
            }

        }
        

    }
    }

}

void rb_delete_fixup(entity ** entity_root, entity * to_fix){
    entity * p; // su slide w == p
    if(to_fix == NULL) return;
    if(to_fix -> parent != NULL){
    if (to_fix -> color == RED || to_fix -> parent == NULL)
        to_fix -> color = BLACK;
    else if (to_fix == to_fix -> parent -> left){
        p = to_fix -> parent -> right;
        if(p != NULL && p-> color == RED){
            p -> color = BLACK;
            to_fix -> parent -> color = RED;
            left_rotate_rb(entity_root, to_fix -> parent);
            p = to_fix -> parent -> right;
        }
        if(p != NULL && (p -> right == NULL|| p->right -> color == BLACK)&&(p-> left == NULL|| p -> left -> color == BLACK)){
            p -> color = RED;
            rb_delete_fixup(entity_root, to_fix -> parent);
        }
        else if(p != NULL &&(p -> right == NULL || p -> right -> color == BLACK)){
            p -> left -> color = BLACK;
            p-> color = RED;
            right_rotate_rb(entity_root, p);
            p = to_fix -> parent -> right;
        }
        if(p != NULL){
            p->color = to_fix -> parent -> color;
            to_fix -> parent -> color = BLACK;
            if(p -> right != NULL)
                p -> right -> color = BLACK;
        }
        
        left_rotate_rb(entity_root, to_fix -> parent);
    }
    else{
        p = to_fix -> parent -> left;
        if(p != NULL && p-> color == RED){
            p -> color = BLACK;
            to_fix -> parent -> color = RED;
            right_rotate_rb(entity_root, to_fix -> parent);
            p = to_fix -> parent -> left;
        }
        if(p != NULL && (p -> right == NULL|| p->right -> color == BLACK)&&(p-> left == NULL|| p -> left -> color == BLACK)){
            p -> color = RED;
            rb_delete_fixup(entity_root, to_fix -> parent);
        }
        else if(p != NULL &&(p -> left == NULL || p -> left -> color == BLACK)){
            p -> right -> color = BLACK;
            p-> color = RED;
            left_rotate_rb(entity_root, p);
            p = to_fix -> parent -> left;
        }
        if(p != NULL){
            p->color = to_fix -> parent -> color;
            to_fix -> parent -> color = BLACK;
            if(p -> left != NULL)
                p -> left -> color = BLACK;
        }
        right_rotate_rb(entity_root, to_fix -> parent);
    }
}
}

/*utility functions */

void sscanf2(char *string, char *command, char * id_orig, char *id_dest, char *id_rel){
    int j =0, i =0;
    fgets(string, MAXSTRING, stdin);
    while(string[i]==' ' || string[i] == '\n')
        i++;
    command[j] = string[i];
    j = 1;
    i ++;
    while(string[i]!=' ' && string[i] != '\n'){
        command[j] = string[i];
        j ++;
        i ++;
    }
    command[j] = '\0';
    if(string[i] == '\n')
        return;
    while(string[i]==' ')
        i++;
    j =0;
    id_orig[j] = string[i];
    j = 1;
    i++;
    while(string[i]!=' ' && string[i] != '\n'){
        id_orig[j] = string[i];
        j ++;
        i ++;
    }
    id_orig[j] = '\0';
    if(string[i] == '\n')
        return;
    while(string[i]==' ')
        i++;
    j =0;
    id_dest[j] =string[i];
    j = 1;
    i++;
    while(string[i]!=' '){
        id_dest[j] = string[i];
        j ++;
        i++;
    }
    id_dest[j] = '\0';
    while(string[i]==' ')
        i++;
    j =0;
    id_rel[j] = string[i];
    j = 1;
    i++;
    while(string[i]!=' ' && string[i] != '\n'){
        id_rel[j] = string[i];
        j ++;
        i++;
    }
    id_rel[j] = '\0';
}

void print_numero(int num, char *string){
    int i = 0;
    while(num > 0){
        string[i] = '0' + num%10;
        num/=10;
        i ++;
    }
    i --;
    while(i >=0){
        putc(string[i], stdout);
        i--;
    }
}

int strcmp(char *s1, char *s2){
    int i = 0;
    for(i = 0; i < SIZE && s1[i]!='\0' && s2[i]!='\0'; i++){
        if (s1[i]!=s2[i]) return (int) s1[i]-s2[i];
    }
    if(s1[i]=='\0'&&s2[i]=='\0') return 0;
    else if(s1[i]=='\0') return -1;
    return 1;
}

void pstrcpy(char *s1, char *s2){
    int i = 0;
    for(i = 0; i<SIZE && s2[i]!= '\0'; i++){
        s1[i]=s2[i];
    }
    if(i<SIZE)
        s1[i]='\0';
    return ;
}

int pstrlen(char * s1){
    int i =0;
    for (i = 0; i < SIZE && s1[i] !='\0'; i ++);
    return i;
}
