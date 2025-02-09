/*
 * @file main.c
 * @author Mattia Callegari (mattia.callegari@polimi.it)
 * @brief Progetto di API 2022 - BST implementation for dictionary purposes,
 *          parsing of words, processing guesses and filtering
 * @version stable
 * @date 2022-08-06
 *
 * @copyright Copyright (c) 2022
 *
 */
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct BSTnode {
  struct BSTnode *right, *left;
  char *word;
} Node;

typedef struct List {
  struct List *next;
  char *word;
} List;

// Lunghezza delle parole
int words_len;

// Contatore parole in lista filtrate
size_t num_filtered = 0;

// Contatore parole in dict
size_t num_nodes_list = 0;

// Buffer per il parsing dell'input
char buffer[256] = {0};

// Variabili per i filtri
char here[256] = {0};
bool not_here[256][128] = {0};
int min_times[128] = {0};
int exact_times[128] = {[0 ... 127] = -1};

// Prototipi funzioni gestione BST
Node *new_node(char *);
void insert(Node **, Node **, char *, int);
int search(Node *, char *);
void stampa(Node *);

// Prototipi funzioni gestione parole filtrate
void insert_filtrate(Node **, char *);
void stampa_lista(List *);
void push(List **, List **, char *);
void pulisci_albero(Node **);
void pulisci_lista(List **);

// Prototipi funzioni gestione partite
void loop_insert_dict(Node **, Node **, int);
void loop_insert_game(Node *, Node *);
int confronto(Node *, Node **, List **, List **, char *, char *, int);
int rispetta_filtro(const char *w);
void reset_filtro();
void game(Node **);

// Funzioni gestione BST

Node *new_node(char *new_word) {
  Node *temp = malloc(sizeof *temp + words_len + 1);
  temp->left = temp->right = NULL;

  temp->word = malloc(words_len + 1);
  strcpy(temp->word, new_word);

  return temp;
}

void insert(Node **root_ref, Node **root2_ref, char *string, int game) {
  Node *z = new_node(string);
  Node *x = *root_ref;
  Node *y = NULL;
  int comp;

  while (x != NULL) {
    // salvo il trailing pointer a x per l'inserimento
    y = x;

    // salvo il risultato del confronto in comp
    comp = strcmp(z->word, x->word);

    // inserimento secondo ordine lessicografico
    if (comp > 0)
      x = x->right;
    else if (comp < 0)
      x = x->left;
  }
  if (y == NULL)
    *root_ref = z;
  else if (comp > 0)
    y->right = z;
  else
    y->left = z;

  if (game) {
    if (rispetta_filtro(string))
      insert_filtrate(root2_ref, z->word);
  }
}

int search(Node *root_ref, char *guess) {
  Node *x = root_ref;

  int comp;

  while (x != NULL) {
    comp = strcmp(guess, x->word);
    if (comp == 0)
      return 1;
    if (comp > 0)
      x = x->right;
    else
      x = x->left;
  }

  return 0;
}

void stampa(Node *root_ref) {
  if (root_ref != NULL) {
    stampa(root_ref->left);
    puts(root_ref->word);
    stampa(root_ref->right);
  }
}

void free_dict(Node *x) {
  if (x != NULL) {
    free_dict(x->right);
    free_dict(x->left);
    free(x->word);
    free(x);
  }
}

// Funzioni gestione albero filtrate e lista temporanea per trasferimento

Node *new_node_filt(char *string) {
  Node *new = malloc(sizeof *new);
  new->left = new->right = NULL;
  new->word = string;
  return new;
}

void insert_filtrate(Node **root_ref, char *string) {
  num_filtered++;
  Node *z = new_node_filt(string);
  Node *x = *root_ref;
  Node *y = NULL;
  int comp;

  while (x != NULL) {
    // salvo il trailing pointer a x per l'inserimento
    y = x;

    // salvo il risultato del confronto in comp
    comp = strcmp(z->word, x->word);

    // inserimento secondo ordine lessicografico
    if (comp > 0)
      x = x->right;
    else if (comp < 0)
      x = x->left;
  }
  if (y == NULL)
    *root_ref = z;
  else if (comp > 0)
    y->right = z;
  else
    y->left = z;
}

void push(List **head_ref, List **tail_ref, char *string) {
  List *new = malloc(sizeof *new);
  new->next = NULL;
  new->word = string;

  if (*head_ref == NULL) {
    (*head_ref) = new;
    (*tail_ref) = new;
  } else {
    (*tail_ref)->next = new;
    (*tail_ref) = new;
  }

  num_nodes_list++;
  num_filtered++;
}

void inOrder(Node *root_ref, List **head_ref, List **tail_ref) {
  if (root_ref != NULL) {
    inOrder(root_ref->left, head_ref, tail_ref);
    if (rispetta_filtro(root_ref->word))
      push(head_ref, tail_ref, root_ref->word);
    inOrder(root_ref->right, head_ref, tail_ref);
  }
}

Node *list_to_bst(List **head_ref, int n) {
  if (n <= 0)
    return NULL;

  Node *left = list_to_bst(head_ref, n / 2);
  Node *root = new_node_filt((*head_ref)->word);

  root->left = left;

  List *temp = (*head_ref);
  (*head_ref) = (*head_ref)->next;
  free(temp);
  num_nodes_list--;

  root->right = list_to_bst(head_ref, n - n / 2 - 1);

  return root;
}

void genera_filtrate(Node *root_ref, Node **filt_ref, List **head,
                     List **tail) {
  num_filtered = 0;
  inOrder(root_ref, head, tail);
  (*filt_ref) = list_to_bst(head, num_nodes_list);
}

void stampa_lista(List *head_ref) {
  while (head_ref != NULL) {
    puts(head_ref->word);
    head_ref = head_ref->next;
  }
}

void filtra_albero(Node **filt_ref, List **head, List **tail) {
  num_filtered = 0;

  inOrder(*filt_ref, head, tail);

  pulisci_albero(filt_ref);

  *filt_ref = list_to_bst(head, num_nodes_list);

  pulisci_lista(head);
}

void pulisci_lista(List **head_ref) {
  List *temp = NULL;
  while (*head_ref != NULL) {
    temp = (*head_ref);
    (*head_ref) = (*head_ref)->next;
    free(temp);
    num_nodes_list--;
  }
}

void pulisci_albero(Node **node) {
  if (*node != NULL) {
    pulisci_albero(&((*node)->left));
    pulisci_albero(&((*node)->right));
    free(*node);
    *node = NULL;
    // num_filtered--;
  }
}

// Funzioni gestione partita

void loop_insert_dict(Node **root_ref, Node **filt_ref, int game) {
  while (fgets(buffer, sizeof buffer, stdin) != 0) {
    if (buffer[0] == '+')
      break;
    buffer[words_len] = 0;
    insert(root_ref, filt_ref, buffer, game);
  }
}

int confronto(Node *root_ref, Node **filt_ref, List **head, List **tail,
              char *g, char *t, int flag) {
  bool controllato[words_len];
  bool consumato[words_len];
  char output[words_len + 1];
  int curr_min_times[128] = {0};
  int count = 0;

  memset(controllato, 0, sizeof controllato);
  memset(consumato, 0, sizeof consumato);
  memset(output, 0, sizeof output);

  for (size_t i = 0; i < words_len; i++) {
    if (g[i] == t[i]) {
      // Aggiorna array di confronto e output
      output[i] = '+';
      controllato[i] = true;
      consumato[i] = true;
      count++;

      // Aggiorna filtri
      here[i] = g[i];
      curr_min_times[(int)g[i]]++;
    } else
      not_here[i][(int)g[i]] = true;
  }

  for (size_t j = 0; j < words_len; j++) {
    if (controllato[j])
      continue;
    for (size_t k = 0; k < words_len; k++) {
      if (consumato[k])
        continue;
      if (g[j] == t[k]) {
        // Aggiorna array di confronto e di output
        output[j] = '|';
        consumato[k] = true;

        // Aggiorna filtri
        curr_min_times[(int)g[j]]++;
        break;
      }
    }

    if (output[j] == 0) {
      // Aggiorna output
      output[j] = '/';

      // Aggiorna filtri
      exact_times[(int)g[j]] = curr_min_times[(int)g[j]];
    }
  }

  for (size_t i = 45; i < 123; i++) {
    if (min_times[i] < curr_min_times[i])
      min_times[i] = curr_min_times[i];
  }

  // Se la parola é stata indovinata ritorna 1 senza stampare
  if (count == words_len)
    return 1;

  if (flag) {
    genera_filtrate(root_ref, filt_ref, head, tail);
  } else
    filtra_albero(filt_ref, head, tail);

  // Stampa il confronto e il numero di parole che rispettano i nuovi filtri
  puts(output);
  printf("%ld\n", num_filtered);
  return 0;
}

int rispetta_filtro(const char *w) {
  int count[128] = {0};

  for (size_t i = 0; i < words_len; i++) {
    count[(int)w[i]]++;
    if (here[i] > 0 && here[i] != w[i])
      return 0;
    if (not_here[i][(int)w[i]])
      return 0;
  }

  for (size_t i = 45; i < 128; i++) {
    if (min_times[i] > count[i])
      return 0;
    if (exact_times[i] >= 0 && exact_times[i] != count[i])
      return 0;
  }

  // Se supero tutti i controlli allora w rispetta i filtri
  return 1;
}

void reset_filtro() {
  for (size_t i = 45; i < 123; i++) {
    min_times[i] = 0;
  }

  for (size_t i = 45; i < 123; i++) {
    exact_times[i] = -1;
  }

  for (size_t i = 0; i < 256; i++) {
    here[i] = 0;
  }

  for (size_t i = 45; i < 123; i++) {
    for (size_t j = 0; j < 256; j++) {
      not_here[j][i] = 0;
    }
  }
}

void game(Node **root_ref) {
  // Stringa contentente la parola da indovinare
  char target[words_len + 1];

  Node *root_filtrate = NULL;
  List *head = NULL;
  List *tail = NULL;

  int flag = 1;

  // Numero di tentativi rimanenti
  int num_guesses = 0;

  num_filtered = 0;

  if (fgets(target, sizeof target, stdin) == 0)
    ;

  if (scanf("%d\n", &num_guesses) <= 0)
    ;

  while (num_guesses > 0) {
    if (fgets(buffer, sizeof buffer, stdin) == 0)
      ;

    // inserito un comando
    if (buffer[0] == '+') {
      // inserito +inserisci_inizio
      if (buffer[1] == 'i') {
        loop_insert_dict(root_ref, &root_filtrate, 1);
      }
      // inserito +stampa_filtrate
      else {
        if (flag)
          stampa(*root_ref);
        else
          stampa(root_filtrate);
      }
      continue;
    }

    buffer[words_len] = 0;

    // processiamo il tentativo
    // Se appartiene faccio il confronto e conto il tentativo
    if (search(*root_ref, buffer)) {
      // se tentativo è corretto, stampo ok e esco dal gioco
      if (confronto(*root_ref, &root_filtrate, &head, &tail, buffer, target,
                    flag)) {
        puts("ok");
        break;
      }
      // Conto il tentativo
      num_guesses--;
      flag = 0;
    }
    // se non appartiene stampo not_exists
    else
      puts("not_exists");
  }

  if (num_guesses == 0)
    puts("ko");

  pulisci_albero(&root_filtrate);
}

int main() {
  Node *root = NULL;

  // leggi lunghezza delle parole
  if (scanf("%d\n", &words_len) == 0)
    ;

  // inserisci parole al dizionario
  loop_insert_dict(&root, NULL, 0);

  do {
    if (buffer[0] == '+' && buffer[1] == 'n') {
      game(&root);
      reset_filtro();
    } else if (buffer[0] == '+' && buffer[1] == 'i')
      loop_insert_dict(&root, NULL, 0);

  } while (fgets(buffer, sizeof buffer, stdin) != 0);

  free_dict(root);

  return 0;
}
