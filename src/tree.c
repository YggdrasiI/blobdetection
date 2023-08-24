#include <stdio.h>
#include <math.h>

#include <assert.h>

//#define INLINE inline
#include "tree.h"
#include "tree_intern.h"

/* Allocate tree struct. If you use
 * the data pointer of the nodes you has to
 * setup/handle the storage for this data
 * separatly.
 */
Tree *tree_create(
    uint32_t size,
    int init_nodes)
{
  // malloc/calloc can return pointer !=NULL for size=0.
  if(size == 0) return NULL;

  Tree *tree = (Tree*) malloc(sizeof(Tree));
  if(tree == NULL) return NULL;

  Node *nodes;
  if (init_nodes == 0) {
    nodes = (Node*) malloc( size*sizeof(Node));
  }else{
    nodes = (Node*) calloc( size, sizeof(Node));
  }
  if(nodes == NULL) { // malloc failed
      free(tree);
      return NULL;
  }

  // Only position to setup read-only variables
  *(Node **)&tree->nodes = nodes;
  *(uint32_t *)&tree->size = size;

  tree->root = tree->nodes;
  if (init_nodes == 0) {
    // Root node initialized as leaf, all other nodes unintialized.
    *tree->root = Leaf;
  }else{
    for( uint32_t i=0; i<size; ++i ) {
      // Setup any non-null values. Currently above calloc is fine…
    }
  }

  return tree;
}


/* Dealloc tree. Attention, target of data pointer is not free'd. */
void tree_destroy(
    Tree **ptree)
{
  if( *ptree == NULL ) return;
  Tree *tree = *ptree;
  if( tree->nodes != NULL ){
    free( tree->nodes );
  }
  free(tree);
  *ptree = NULL;
}


#ifdef TREE_REDUNDANT_INFOS
/* Eval height and number of children for each Node */
void tree_gen_redundant_information(
    Node * const root)
{
  // Remove height information about this subtree from parent
  Node *node = root;
  if( node->parent ){
      --node->parent->width; // Will be increased again later.
  }
  while( node->parent ){
    node->parent->height = 1; // Not 0… node is still present.
    Node *cur = node->parent->child;
    while(cur){
      if(cur!= node){
        if( node->parent->height < cur->height+1 ){
          node->parent->height = cur->height+1;
        }
      }
      cur = cur->sibling;
    }
    node = node->parent;
  }

  // Now handle this subtree.
  node = root;
  do{
    /* Reset values */
    node->height = 0;
    node->width = 0;

    if( node->parent ){
      ++node->parent->width;
      if( node->parent->height < node->height+1 ){
        node->parent->height = node->height+1;
      }
    }

    /* To to next node. update parent node if possible */
    if( node->child != NULL ){
      node = node->child;
      continue;
    }
    if( node->sibling != NULL ){
      node = node->sibling;
      continue;
    } 
    while( node->parent != NULL ){
      node = node->parent;
      if( node->parent ){
        if( node->parent->height < node->height+1 ){
          node->parent->height = node->height+1;
        }
      }
      if( node == root) break;
      if( node->sibling != NULL ){
        node = node->sibling;
        break;
      }
    }
  }while( node != root );

}


void _tree_gen_redundant_information_recursive(
    Node* root,
    uint32_t *pheight,
    uint32_t *psiblings);

/* Eval height and number of children for each Node */
void tree_gen_redundant_information_recursive(
    Node* root)
{
    _tree_gen_redundant_information_recursive(root, NULL, NULL);
}

void _tree_gen_redundant_information_recursive(
    Node* root,
    uint32_t *pheight,
    uint32_t *psiblings)
{
  root->width = 0;
  if ( psiblings != NULL ){
    (*psiblings)++; //update number of children for parent node
  }
  if( root->child != NULL ){
    uint32_t height2=0;
    _tree_gen_redundant_information_recursive(root->child, &height2, &root->width);
    if( pheight != NULL) { 
      if( *pheight < height2+1 ) *pheight = height2+1; //update height of parent node
    }
  }
  if( root->sibling != NULL ){
    _tree_gen_redundant_information_recursive(root->sibling, pheight, psiblings);
  }
}
#endif

// Update of height after one branch grows
// Here, c has to be new/updated child of p!
static inline void _update_heights_after_add(
    Node *p,
    uint32_t h /* Increased height of existing node or height of new node */)
{
  ++h;
  while( p != NULL && p->height < h ){
    p->height = h;
    p=p->parent;
    ++h;
  }
}

// Update of height after one branch shrinked
// Here, c has to be old/outdated child of p! (To check
// if p->height was ruled by c's values.)
static inline void _update_heights_after_shrink(
    Node *p,
    uint32_t previous_height_of_removed_or_changed_child)
{

  // New height is maximum of all children except the removed/shrinked
  // one. We need to go upwards until the node is not
  // in the critical branch.
  while( p != NULL && p->height == previous_height_of_removed_or_changed_child+1 ){
    uint32_t new_max_height_child = 0;
    Node *c=p->child;
    while (c != NULL){
      if (new_max_height_child < c->height){
        new_max_height_child = c->height;
      }
      c = c->sibling;
    }

    previous_height_of_removed_or_changed_child = p->height; // for next loop step
    p->height = new_max_height_child + 1;
    p=p->parent;
  }
}

int tree_add_child(
    Node * const parent,
    Node * const node)
{
  if (parent == NULL) return -2;
  if (node == NULL) return -1;
  assert(node->sibling == NULL);

  if( parent->child == NULL ){
    parent->child = node;
  }else{
    Node *cur = parent->child;
    while( cur->sibling != NULL ){
      cur = cur->sibling;
    }
    cur->sibling = node;
  }
  node->parent = parent;

#ifdef TREE_REDUNDANT_INFOS
  //update redundant information
  parent->width++;
  _update_heights_after_add(parent, node->height);
#endif
  return 0;
}


int tree_add_siblings(
    Node * const parent,
    Node * const sibling)
{
  if (parent == NULL) return -2; // Use tree_release_child()
  if (sibling == NULL) return -1;


  // Backup leftmost sibling before releasing previous parent info
  Node * const leftmost_sib=sibling->parent==NULL?sibling:sibling->parent->child;

  Node *prev_parent = leftmost_sib->parent;
#ifdef TREE_REDUNDANT_INFOS
    // Backup values (**)
    const uint32_t num_siblings_prev_parent=prev_parent?prev_parent->width:0;
    const uint32_t height_prev_parent=prev_parent?prev_parent->height:0;
#endif

  // Removing child info in previous parent node
  if (prev_parent != NULL){
    // Note that we cannot use tree_release_child() because it would clear
    // the sibling releation between the looped nodes!

    // Loop commented out, because we will set the parent again, see below.
#if 0
    Node *sib = leftmost_sib;
    while (sib != NULL){
      sib->parent = NULL;
      sib = sib->sibling;
    }
#endif
    prev_parent->child = NULL;

#ifdef TREE_REDUNDANT_INFOS
    // update redundant information
    prev_parent->width = 0;
    prev_parent->height = 0;
    _update_heights_after_shrink(prev_parent->parent, height_prev_parent);
#endif
  }

  // Attach leftmost new sibling to rightmost old sibling
  if( parent->child == NULL ){
    parent->child = leftmost_sib;
  }else{
    Node *cur = parent->child;
    while( cur->sibling != NULL ){
      cur = cur->sibling;
    }
    cur->sibling = leftmost_sib;
  }

  //set new parent of children
  Node *sib = leftmost_sib;
  while( sib ){
    sib->parent = parent;
    sib=sib->sibling;
  }

#ifdef TREE_REDUNDANT_INFOS
  /*
  // Counting New child and get maximal height
  uint32_t num_siblings=parent->width;
  uint32_t max_new_height=parent->height;
  Node *sib = leftmost_sib;
  while( sib ){
    sib->parent = parent;
    num_siblings++;
    if(sib->height > max_new_height){
      max_new_height=sib->height;
    }
    sib=sib->sibling;
  }

  //update redundant information
  parent->width+=num_siblings;
  parent->height = max_new_height;
  _update_heights_after_add(p->parent, p->height);
  */
  // Relying on (**) saves some operation
  parent->width+=num_siblings_prev_parent;
  if (parent->height<height_prev_parent) parent->height=height_prev_parent;
  _update_heights_after_add(parent->parent, parent->height);
#endif

  return 0;
}

/* Remove child from its parent node.
 *
 */
int tree_release_child(
    Node * const child,
    int leaf_only)
{
  if (child == NULL) return -2;
  if (leaf_only && child->child) return -3;
  Node *parent = child->parent;
  if (parent == NULL){
    if (child->sibling){
#ifndef NDEBUG
      fprintf(stderr, "tree_release_child() called for root node with sibling"
          "This indicates that tree has no integrity!");
#endif
      return -1;
    }
    return 0; // nothing to do for root nodes without sibling.
  }
  /*if (parent == NULL){
      // No parent node. We do not return here because
      // the (root) node can still have siblings
  }else */if (parent->child == child) {
      // first child case
     parent->child = child->sibling;
  }else{
      // other child cases
      Node *sibling = parent->child;
      while (sibling){
          if (sibling->sibling == child) {
              sibling->sibling = child->sibling;
              break;
          }
          sibling = sibling->sibling;
      }
  }

#ifdef TREE_REDUNDANT_INFOS
  //update redundant information
  parent->width--;
  _update_heights_after_shrink(parent, child->height);
#endif

  // Release links from child to it parent/right siblings, too.
  child->parent = NULL;
  child->sibling = NULL;
  return 0;
}

int tree_swap_siblings(
    Node * const child1,
    Node * const child2)
{
  if (child1 == NULL) return -4; // use tree_release_child()
  if (child2 == NULL) return -3; // use tree_release_child()
  Node *parent = child1->parent;
  if (parent == NULL) return -2;
  if (parent != child2->parent) return -1; // not the same parent.

  if (child1 == child2) return 0; // Avoids checks for some cycle references

  // Search left siblings. If NULL after serach childX is leftmost node
#if 0
  Node *left_sib1=left_sibling(child1);
  Node *left_sib2=left_sibling(child2);
#else
  Node *left_sib1 = NULL, *left_sib2 = NULL;
  Node *sib = child1->parent->child;
  Node *ref, **refB;
  while (sib){
    if (sib->sibling == child1){
      left_sib1=sib;
      ref=child2; refB=&left_sib2;
      break;
    }
    if (sib->sibling == child2){
      left_sib2=sib;
      ref=child1; refB=&left_sib1;
      break;
    }
    sib = sib->sibling;
  }
  while (sib){
    if (sib->sibling == ref){
      *refB=sib;
      break;
    }
    sib = sib->sibling;
  }
#endif

  // Swap right siblings;
  Node *tmp = child1->sibling;
  child2->sibling = child1->sibling;
  child1->sibling = tmp;

  // Swap references in left siblings/parent
  if (left_sib1){
    left_sib1->sibling = child2;
  }else{
    parent->child = child2;
  }
  if (left_sib2){
    left_sib2->sibling = child1;
  }else{
    parent->child = child1;
  }

#ifdef TREE_REDUNDANT_INFOS
  // Nothing to do in this case/operation.
#endif
  return 0;
}

/* Replaces node1 by node2 and vice versa. */
int tree_swap_nodes(
    Node * const node1,
    Node * const node2)
{
  if (node1 == NULL) return -2; // use tree_release_child()
  if (node2 == NULL) return -3; // use tree_release_child()

  // Swap parents
  Node *parent1 = node1->parent;
  Node *parent2 = node2->parent;
  if (parent1 == parent2
    /* swapping siblings just works for non-root nodes! */
    && parent1 != NULL ){
    return tree_swap_siblings(node1, node2);
  }
  node1->parent = parent2;
  node2->parent = parent1;

  // Search left siblings of both nodes
  Node *left_sib1 = tree_left_sibling(node1);
  Node *left_sib2 = tree_left_sibling(node2);

  // Swap right siblings;
  Node *tmp = node1->sibling;
  node2->sibling = node1->sibling;
  node1->sibling = tmp;

  // Swap references in left siblings/parent
  if (left_sib1){
    left_sib1->sibling = node2;
  }else if(parent1){
    parent1->child = node2;
  }
  if (left_sib2){
    left_sib2->sibling = node1;
  }else if(parent2){
    parent2->child = node1;
  }

#ifdef TREE_REDUNDANT_INFOS
  //update redundant information
  if( node1->height < node2->height ){
    _update_heights_after_add(parent1, node2->height);
    _update_heights_after_shrink(parent2, node2->height);
  }
  if( node1->height > node2->height ){
    _update_heights_after_shrink(parent1, node1->height);
    _update_heights_after_add(parent2, node1->height);
  }
#endif

  return 0;
}

uint32_t tree_number_of_nodes(
    Node *root)
{
 uint32_t n = 1;
 Node *cur = root->child;
 while( cur != NULL ){
   n++;
   if( cur->child != NULL ) cur = cur->child;
   else if( cur->sibling != NULL ) cur = cur->sibling;
   else{
     cur = cur->parent;
     while(cur != root && cur->sibling == NULL) cur = cur->parent;
     cur = cur->sibling;
   }
 }
 return n;
}


void _tree_print(
    const Tree * tree,
    const Node *node,
    int32_t shift)
{
  // TODO
  int32_t i;
  int32_t shift2=0;
  // Gen unique and stable id for nodes in tree-struct array
  int32_t id = (node>=tree->root)?
    ((node - tree->root)% tree->size):
    ((tree->root - node)% tree->size + tree->size);
#ifdef TREE_REDUNDANT_INFOS
#if 0
  printf("%3i• (w:%2i, h:%2i) ", id, node->width, node->height);
  shift2+=19;
#else
  printf("%3i• ", id);
  shift2+=5;
#endif
#else
  printf("%3i• ", id);
  shift2+=5;
#endif


  if( node->child != NULL){
    printf("→");
    _tree_print(tree, node->child, shift+shift2);
  }else{
    printf("\n");
  }

  if( node->sibling != NULL){
  //  printf("\n");
    for(i=0; i<shift-1; i++) printf(" ");
    printf("↘");
    _tree_print(tree, node->sibling, shift);
  }
}

void tree_print(
    const Tree * tree,
    const Node *subtree_root,
    int32_t shift)
{
  if( subtree_root != NULL ){
    assert(subtree_root >= tree->nodes);
    assert(subtree_root < tree->nodes + tree->size);
  }
  _tree_print(tree, subtree_root?subtree_root:tree->root, shift);
}



/* Generate unique id for sorting trees.
 * [DE] Wird ein Baum durchlaufen und für jeden Schritt angegeben, ob als nächstes ein
 * Kindknoten oder der nächste Geschwisterknoten auf x-ter Ebene ist, so entsteht eine
 * eindeutige Id eines Baumes. Diese kann später für vergleiche genutzt werdne.
 * Kann man das noch komprimieren, wenn man als Basis die maximale Tiefe wählt?!
 *
 * */
void _gen_tree_id(
    Node *root,
    uint32_t **id,
    uint32_t *d)
{
  if( root->child != NULL ){
    //printf("o ");
    **id = 0;
    (*id)++; //set pointer to next array element
    _gen_tree_id(root->child, id, d);
  }else{
    *d = root->height; //store height of leaf.
  }
  if( root->sibling != NULL ){
    //print difference from last leaf and this node and add 1
    //printf("%u ", root->height -*d +1 );
    **id = (root->height - *d + 1);
    (*id)++; //set pointer to next array element
    *d=root->height;
    _gen_tree_id(root->sibling, id, d);
  }
}


/* Generate Unique Number [xyz...] for node
 * Preallocate id-array with #nodes(root).
 * */
void tree_generate_id(
    Node *root,
    uint32_t* id,
    uint32_t size)
{
 uint32_t last_height=0;
 //store size of array in first element
 *id = size;
 id++;
 _gen_tree_id(root, &id, &last_height);
 printf("\n");
}


#if 0
/* tree_hashval, (NEVER FINISHED)
 * Berechnet für einen Baum eine Id, die eindeutig ist,
 * wenn die Bäume eine bestimmte Struktur einhalten.
 * Die Struktur der Bäume (z.B. max. Anzahl der Kinder)
 *  ist grob einstellbar.
 */
static const uint32_t TREE_CHILDREN_MAX = 5;
static const uint32_t TREE_DEPTH_MAX = 5; //height of root is 0.

static uint32_t tree_hashval( Node *root){
  return -1;
}
#endif





// Debug/Helper-Functions

int8_t * debug_getline(void) 
{
    int8_t * line = (int8_t*) malloc(100), * linep = line;
    size_t lenmax = 100, len = lenmax;
    int32_t c;

    if(line == NULL)
        return NULL;

    for(;;) {
        c = fgetc(stdin);
        if(c == EOF)
            break;

        if(--len == 0) {
            size_t dist = (line - linep);
            int8_t * linen = (int8_t*) realloc(linep, lenmax *= 2);
            len = lenmax;

            if(linen == NULL) {
                free(linep);
                return NULL;
            }
            line = linen + dist;
            linep = linen;
        }

        if((*line++ = c) == '\n')
            break;
    }
    *line = '\0';
    return linep;
}


void debug_print_matrix(
    uint32_t* data,
    uint32_t w, uint32_t h,
    BlobtreeRect roi,
    uint32_t gridw, uint32_t gridh)
{
  uint32_t i, j, wr, hr, w2, h2;
  uint32_t d;
  wr = (roi.width-1) % gridw;
  hr = (roi.height-1) % gridh;
  w2 = roi.width - wr;
  h2 = roi.height - hr;
  for(i=roi.y; i<roi.y+h2; i+=gridh){
    for(j=roi.x; j<roi.x+w2; j+=gridw){
      d = *(data+i*w+j);
      //printf("%u ", d);
      //printf("%s", d==0?"■⬛":"□");
      //printf("%s", d==0?"✘":" ");
      if(d>0){
        printf("%3u", d);
      }else{
        printf("   ");
      }
    }
    j-=gridw-wr;

    if(w2<roi.width){
      for(; j<roi.x+roi.width; j+=1){
        d = *(data+i*w+j);
        if(d>0){
          printf("%3u", d);
        }else{
          printf("   ");
        }
      }
    }

    printf("\n");
  }

  i-=gridh-hr;
  if( h2 < roi.height ){
    for(; i<roi.y+roi.height; i+=1){
      for(j=roi.x; j<roi.x+w2; j+=gridw){
        d = *(data+i*w+j);
        if(d>0){
          printf("%3u", d);
        }else{
          printf("   ");
        }
      }
      j-=gridw-wr;

      if(w2<roi.width){
        for(; j<roi.x+roi.width; j+=1){
          d = *(data+i*w+j);
          if(d>0){
            printf("%3u", d);
          }else{
            printf("   ");
          }
        }
      }
      printf("\n");
    }
  }
  printf("\n");
}


void debug_print_matrix2(
    uint32_t* ids,
    uint32_t* data,
    uint32_t w, uint32_t h,
    BlobtreeRect roi,
    uint32_t gridw, uint32_t gridh,
    int8_t b_twice)
{
  uint32_t i, j, wr, hr, w2, h2;
  uint32_t d;
  wr = (roi.width-1) % gridw;
  hr = (roi.height-1) % gridh;
  w2 = roi.width - wr;
  h2 = roi.height - hr;
  for(i=roi.y; i<roi.y+h2; i+=gridh){
    for(j=roi.x; j<roi.x+w2; j+=gridw){
      if( *(ids+i*w+j) > 0 ){
        d = *(data+*(ids+i*w+j));
        if(b_twice) d=*(data+d);
        //printf("%s%u", d<10&&d>=0?" ":"", d);
        printf("%3u", d);
      }else{
        //printf("  ");
        printf("   ");
      }
    }
    j-=gridw-wr;

    if(w2<roi.width){
      for(; j<roi.x+roi.width; j+=1){
        if( *(ids+i*w+j) > 0 ){
          d = *(data+*(ids+i*w+j));
          if(b_twice) d=*(data+d);
          //printf("%s%u", d<10&&d>=0?" ":"", d);
          printf("%3u", d);
        }else{
          printf("   ");
        }
      }
    }

    printf("\n");
  }

  i-=gridh-hr;
  if( h2 < roi.height ){
    for(; i<roi.y+roi.height; i+=1){
      for(j=roi.x; j<roi.x+w2; j+=gridw){
        if( *(ids+i*w+j) > 0 ){
          d = *(data+*(ids+i*w+j));
          //printf("%s%u", d<10&&d>=0?" ":"", d);
          printf("%3u", d);
        }else{
          printf("   ");
        }
      }
      j-=gridw-wr;

      if(w2<roi.width){
        for(; j<roi.x+roi.width; j+=1){
          if( *(ids+i*w+j) > 0 ){
            d = *(data+*(ids+i*w+j));
            printf("%s%u", d<10&&d>=0?" ":"", d);
          }else{
            printf("  ");
          }
        }
      }
      printf("\n");
    }
  }
  printf("\n");
}


void debug_print_matrix_char(
    uint8_t * data,
    uint32_t w, uint32_t h,
    BlobtreeRect roi,
    uint32_t gridw, uint32_t gridh)
{
  uint32_t i, j, wr, hr, w2, h2;
  uint32_t d;
  wr = (roi.width-1) % gridw;
  hr = (roi.height-1) % gridh;
  w2 = roi.width - wr;
  h2 = roi.height - hr;
  for(i=roi.y; i<roi.y+h2; i+=gridh){
    for(j=roi.x; j<roi.x+w2; j+=gridw){
      d = *(data+i*w+j);
      //printf("%u ", d);
      //printf("%s", d==0?"■⬛":"□");
      //printf("%s", d==0?"✘":" ");
      if(d>0)
        //printf("%s%u", d<10&&d>=0?" ":"", d);
        printf("%2u", d);
      else
        printf("  ");
    }
    j-=gridw-wr;

    if(w2<roi.width){
      for(; j<roi.x+roi.width; j+=1){
        d = *(data+i*w+j);
        if(d>0)
          //printf("%s%u", d<10&&d>=0?" ":"", d);
          printf("%2u", d);
        else
          printf("  ");
      }
    }

    printf("\n");
  }

  i-=gridh-hr;
  if( h2 < roi.height ){
    for(; i<roi.y+roi.height; i+=1){
      for(j=roi.x; j<roi.x+w2; j+=gridw){
        d = *(data+i*w+j);
        if(d>0)
          //printf("%s%u", d<10&&d>=0?" ":"", d);
          printf("%2u", d);
        else
          printf("  ");
      }
      j-=gridw-wr;

      if(w2<roi.width){
        for(; j<roi.x+roi.width; j+=1){
          d = *(data+i*w+j);
          if(d>0)
            //printf("%s%u", d<10&&d>=0?" ":"", d);
            printf("%2u", d);
          else
            printf("  ");
        }
      }
      printf("\n");
    }
  }
  printf("\n");
}

Node * tree_left_sibling(
    Node *n)
{
  if(n->parent == NULL) return NULL;

  Node *sib = n->parent->child;
  while (sib){
    if (sib->sibling == n) return sib;
    sib = sib->sibling;
  }
  return NULL;
}

/* Shortcut for 
 * int err = foobar(…)
 * if (err<0) return err;
 *
 * For debugging:
 *   Insert assert(0) call to find the position where the error occours.
 */
#define RETURN_ON_INTEGRITY_ERR(code) \
{ int _err = code; \
    if (_err<0) { \
      assert(_err == 0); \
      return _err; \
    } \
}

/* Return value:
 *  >= 0: Number of children <= TREE_CHILDREN_MAX
 *  < 0: Error status
 */
int _check_too_many_children(Node * const parent){
  if (parent == NULL) RETURN_ON_INTEGRITY_ERR(TREE_ERROR_NODE_IS_NULL);

  Node *n = parent->child;
  uint32_t num_siblings=0;
  while (n) {
    ++num_siblings;
    if(num_siblings > TREE_MAX_CHILDREN)
      RETURN_ON_INTEGRITY_ERR(TREE_ERROR_TOO_MANY_CHILDREN);
    n=n->sibling;
  }
  return num_siblings;
}

/* Returns 0 if at most TREE_MAX_CHILDREN (right) siblings have same parent.*/
int _check_parent_relation(Node *n){
  Node * const parent = n->parent;
  uint32_t num_siblings=0;
  while (n) {
    ++num_siblings;
    if(num_siblings > TREE_MAX_CHILDREN){
      RETURN_ON_INTEGRITY_ERR(TREE_ERROR_TOO_MANY_CHILDREN);
    }
    if(n->parent != parent){
      RETURN_ON_INTEGRITY_ERR(TREE_ERROR_NODE_HAS_WRONG_PARENT);
    }
    n=n->sibling;
  }
  return 0;
}

/* Searches first entry of a silbing sequence which is 
 * linked as silbing twice. (Thus, looping over silbings never ends).
 *
 * Return value:
 *  ret < 0: Error occoured.
 *  ret = 0: if no duplicate can be found (best case).
 *  ret > 0: cycle length until *pOut will be reached again.
 *
 * Assumptions:
 *   • First TREE_MAX_CHILDREN siblings share same parent (or NULL).
 *
 * Task: Find X with two predecessors N_a, N_b
 *
 *    N_0 →    …  → N_a → X → …
 *  (Non-cycle prelude)   ↑   ↓  (Cycle of length L)
 *                      N_b ← …
 *
 * Solution: 
 *  0. N_0 is start->parent->child.
 *  1. Assume parent-property is constant for input.
 *  2. Loop trough nodes and save predecessor relation in node.parent.
 *  3. If node is reached which was already changed, we found N_b, N_a and X.
 *  4. Reverse loop from N_b to N_0 and restore original parent node.
 *     We we reaching N_a, we also got L.
 *
 *
 *  If N_a is its own sibling, then N_a != X, X = N_b.
 */
int _search_leftmost_silbing_duplicate(
    Node * const node,
    Node **pOut)
{
  *pOut = NULL;
  if (node == NULL) RETURN_ON_INTEGRITY_ERR(TREE_ERROR_NODE_IS_NULL);

  /* Shortcut for trivial case
   * This avoids recursion problem of line (***) for step 4
   * if starting node is its own sibling, node->sibling == node.
   */
  if (node->sibling == node) {
    *pOut = node;
    return 1; // cycle length/distance to first duplicate
  }

  Node * const parent = node->parent; // Backup for step 4. Can be NULL
  Node * const leftmost_child = node->parent?node->parent->child:node;
  Node *N_a = leftmost_child->parent->child;
  Node *N_b = NULL, *X = NULL;
  Node *cur = leftmost_child;
  uint32_t num_siblings=0;
  leftmost_child->parent = NULL; // Endpoint of step 4
  while(cur->sibling) {
    ++num_siblings;
    if(num_siblings > TREE_MAX_CHILDREN){
      RETURN_ON_INTEGRITY_ERR(TREE_ERROR_TOO_MANY_CHILDREN);
    }
    if( cur->sibling->parent != parent ){
      // Step 3
      N_a = cur->sibling->parent;
      X = cur->sibling;
      N_b = cur;
      break;
    }
    // Step 2
    cur->sibling->parent = cur; // Save left sibling, (***)
    cur=cur->sibling;
  }
  // Step 4
  int cycle_len=0;
  {
    assert(cur == N_b);
    _unused(N_b);
    // Reset values in cycle
    while(cur) {
      Node *tmp_left=cur->parent;
      cur->parent = parent;
      cur = tmp_left;
      ++cycle_len;

      if (cur == N_a){
        break;
      }
    }
    // Reset values in prelude.
    assert(cur == N_a);
    while(cur) {
      Node *tmp_left=cur->parent;
      cur->parent = parent;
      cur = tmp_left;
    }
  }

  *pOut = X;
  return cycle_len;
}

int _integrity_check_for_leftmost_nodes(
    Node * const node)
{
  /* Non-root nodes have always a parent */
  if( node->parent == NULL)
    RETURN_ON_INTEGRITY_ERR(TREE_ERROR_PARENT_NODE_IS_NULL);

  /* Check up level reference.
   *
   *  ↗ C_1, …, C_n
   * P
   *  ↘ C'_1, …, C'_m
   */
  if(node != node->parent->child)
    RETURN_ON_INTEGRITY_ERR(TREE_ERROR_NODE_IS_PARENT_FOR_NONSIBLING);

  // Count siblings of 'node'
  int num_siblings_or_err = _check_too_many_children(node->parent);

  // Siblings share same parent.
  int err_parent_relation = _check_parent_relation(node);

  if (num_siblings_or_err < 0 && !err_parent_relation){
    // Check why it's too many children. Just too many elements or
    // a cycle?!
    Node *duplicate;
    int duplicate_err = _search_leftmost_silbing_duplicate(node, &duplicate);
    if (duplicate_err) {
      return duplicate_err;
    }
    return num_siblings_or_err;
  }

  if (err_parent_relation) return err_parent_relation;


#ifdef TREE_REDUNDANT_INFOS
  if (num_siblings_or_err >= 0){
    if (node->parent->width != num_siblings_or_err) {
      RETURN_ON_INTEGRITY_ERR(TREE_ERROR_NODE_WIDTH_WRONG);
    }
    if (node->parent->width > TREE_MAX_CHILDREN) {
      RETURN_ON_INTEGRITY_ERR(TREE_ERROR_NODE_WIDTH_EXCEEDED);
    }
  }
#endif

  return 0;
}

int _compare_height_of_parent_and_its_children(
    Node * const parent)
{
  uint32_t max_height = 0;
  Node *cur = parent->child;
  while(cur){
    if (max_height < cur->height + 1){
      max_height = cur->height + 1;
    }
    cur = cur->sibling;
  }
  if (max_height != parent->height){
    RETURN_ON_INTEGRITY_ERR(TREE_ERROR_NODE_HEIGHT_WRONG);
  }

  if (max_height > TREE_MAX_HEIGHT){
    RETURN_ON_INTEGRITY_ERR(TREE_ERROR_NODE_HEIGHT_EXCEEDED);
  }

  return 0;
}

int _integrity_check_for_rightmost_nodes(
    Node * const node)
{
#ifdef TREE_REDUNDANT_INFOS
  //Here, all child nodes node->parent were
  //already checked. We can rely on their ->height value.
  RETURN_ON_INTEGRITY_ERR(_compare_height_of_parent_and_its_children(node->parent));

  return 0;
#endif
}

int _leaf_integrety_check(
    Node * const leaf,
    uint32_t depth)
{
  assert(leaf->child == NULL);
#ifdef TREE_REDUNDANT_INFOS
  if (leaf->width != 0)
    RETURN_ON_INTEGRITY_ERR(TREE_ERROR_NODE_WIDTH_WRONG);
  if (leaf->height != 0)
    RETURN_ON_INTEGRITY_ERR(TREE_ERROR_NODE_HEIGHT_WRONG);
#endif

  return 0;
}

int tree_integrity_check(
    Node *node)
{
  if (node == NULL) RETURN_ON_INTEGRITY_ERR(TREE_ERROR_NODE_IS_NULL);
  if (node->parent == NULL && node->sibling){
    RETURN_ON_INTEGRITY_ERR(TREE_ERROR_ROOT_HAS_SIBLING);
  }

  /* With this variable we track if the complete tree
   * exceeded the maximal depth.
   * This counting is orthogonal to (optional) Node.height values.
   */
  uint32_t depth = 0;

  // If we starting in a subtree add the actual depth.
  Node *p = node->parent;
  while(p && depth < TREE_MAX_HEIGHT){
    ++depth;
    p = node->parent;
  }
  if( depth >= TREE_MAX_HEIGHT ){
    RETURN_ON_INTEGRITY_ERR(TREE_ERROR_TOO_MANY_LEVELS); // or cycle in parent-relation
  }
  const uint32_t start_node_depth = depth;

  Node *cur = node->child;
  ++depth;
  while(cur){
    // Checks on leftmost children 
    if( cur == cur->parent->child ){
      RETURN_ON_INTEGRITY_ERR(_integrity_check_for_leftmost_nodes(cur));
    }

#ifdef TREE_REDUNDANT_INFOS
    // Other checks.
    if (cur->height > TREE_MAX_HEIGHT)
      RETURN_ON_INTEGRITY_ERR(TREE_ERROR_TOO_MANY_LEVELS);
    if (cur->height + 1 > cur->parent->height)
      RETURN_ON_INTEGRITY_ERR(TREE_ERROR_NODE_HEIGHT_WRONG);
#endif

    // Find next node
    if(cur->child){
      cur = cur->child;

      ++depth;
      if(depth >= TREE_MAX_HEIGHT)
        RETURN_ON_INTEGRITY_ERR(TREE_ERROR_TOO_MANY_LEVELS);

      continue;
    }

    // Here, cur has to be a leaf
    RETURN_ON_INTEGRITY_ERR(_leaf_integrety_check(cur, depth));

    if(cur->sibling){
      cur = cur->sibling;
      continue;
    }

    // Go upwards
    while(cur->parent){
      RETURN_ON_INTEGRITY_ERR(_integrity_check_for_rightmost_nodes(cur));

      cur = cur->parent;

      --depth;
      uint32_t relative_depth = depth - start_node_depth; // Check for underflow
      assert(relative_depth <= TREE_MAX_HEIGHT); // Failing indicates error in this function, but probably not tree structure.
      _unused(relative_depth);

      if(cur == node){ // Loop has visited all nodes of (sub-)tree.
        cur = NULL;
        break; 
      }
      if(cur->sibling){
        cur = cur->sibling;
        break;
      }
    }
  }

  return 0;
}

void tree_print_integrity_check(
    Node *root)
{
  int err = tree_integrity_check(root);
  if (err == 0){
    fprintf(stderr, "Tree has integrity. :-)\n");
    return;
  }

  // Print error message
  fprintf(stderr, "Tree does not have integrity!\n Error: ");
  fprintf_enum_name(stderr, err);
  fprintf(stderr, "\n");

  /*
  switch( err ) {
    case TREE_ERROR_ROOT_HAS_SIBLING:
      fprintf(stderr,
          "Error %d: Tree contains node with too many children.\n",
          err);
      break;
    case TREE_ERROR_NODE_IS_NULL:
      fprintf(stderr,
          "Error %d: Unexpected NULL reference.\n", 
          err);
      break;
    default:
      fprintf(stderr, "Tree does not have integrity. Error: %d",
          err);
      break;
  }
  */
}
