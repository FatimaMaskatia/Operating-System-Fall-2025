//#include "types.h"
//#include "stat.h"
//#include "user.h"

#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h" 

#define PAGE 4096
#define MAX_PAGES 8192
#define PREFIX "This may help."
#define PREFIX_LEN 15   // length of "This may help." without NUL
#define SECRET_OFFSET 16
#define MAX_SECRET 128

static int
is_alnum_char(char c) {
  if (c >= '0' && c <= '9') return 1;
  if (c >= 'A' && c <= 'Z') return 1;
  if (c >= 'a' && c <= 'z') return 1;
  return 0;
}

/* copy up to `maxlen` alnum chars starting at addr p+start, but stop at page boundary */
static int
copy_alnum_from(char *p, int start, int page_remain, char *out, int maxlen) {
  int i = 0;
  while (i < maxlen && start + i < page_remain && is_alnum_char(p[start + i])) {
    out[i] = p[start + i];
    i++;
  }
  out[i] = 0;
  return i;
}

/* Try to extract secret starting at (page_base + pos_in_page).
   If secret continues into next page, this function will copy only from current page.
   Cross-page secret continuation is handled when scanning the next page's previous-page logic. */
static int
extract_secret_in_page(char *page_base, int pos_in_page, char *out, int maxlen) {
  return copy_alnum_from(page_base, pos_in_page, PAGE, out, maxlen);
}

int
main(int argc, char *argv[])
{
  char *page = 0;
  char *prev_page = 0;
  char out[MAX_SECRET];

  for (int pi = 0; pi < MAX_PAGES; pi++) {
    page = sbrk(PAGE);
    if (page == (char*)-1) break;

    // 1) scan inside this page for the prefix entirely within page
    for (int off = 0; off + PREFIX_LEN + SECRET_OFFSET < PAGE; off++) {
      int match = 1;
      for (int j = 0; j < PREFIX_LEN; j++) {
        if (page[off + j] != PREFIX[j]) { match = 0; break; }
      }
      if (!match) continue;

      // secret starts at off + SECRET_OFFSET
      int sstart = off + SECRET_OFFSET;
      int got = extract_secret_in_page(page, sstart, out, MAX_SECRET - 1);
      if (got > 0) {
        printf("%s\n", out);
        exit(0);
      }
      // If got == 0, the secret might start here but continue into the next page.
      // That case will be handled when the next page is allocated:
      // then the prefix crossing logic below (prev_page check) will see it.
    }

    // 2) scan for prefix that starts in previous page and continues into this page
    if (prev_page) {
      int min_prev_start = PAGE - (PREFIX_LEN - 1);
      if (min_prev_start < 0) min_prev_start = 0;
      for (int prev_off = min_prev_start; prev_off < PAGE; prev_off++) {
        int tail_len = PAGE - prev_off;           // how many bytes of prefix are in prev_page
        int need_from_curr = PREFIX_LEN - tail_len; // how many bytes must come from current page
        if (need_from_curr <= 0 || need_from_curr > PREFIX_LEN) continue;

        int match = 1;
        // check tail of prev_page
        for (int j = 0; j < tail_len; j++) {
          if (prev_page[prev_off + j] != PREFIX[j]) { match = 0; break; }
        }
        if (!match) continue;
        // check head of current page
        for (int j = 0; j < need_from_curr; j++) {
          if (page[j] != PREFIX[tail_len + j]) { match = 0; break; }
        }
        if (!match) continue;

        // prefix found starting at prev_off in prev_page.
        // The secret begins at global offset (prev_off + SECRET_OFFSET).
        int secret_global_pos = prev_off + SECRET_OFFSET;
        if (secret_global_pos < PAGE) {
          // secret starts in prev_page
          int got = extract_secret_in_page(prev_page, secret_global_pos, out, MAX_SECRET - 1);
          // if it spills into current page, append from current page head
          if (got < MAX_SECRET - 1 && secret_global_pos + got >= PAGE) {
            int j = 0;
            while (got < MAX_SECRET - 1 && j < PAGE && is_alnum_char(page[j])) {
              out[got++] = page[j++];
            }
            out[got] = 0;
          }
          if (got > 0) {
            printf("%s\n", out);
            exit(0);
          }
        } else {
          // secret starts in current page at position (secret_global_pos - PAGE)
          int pos_in_curr = secret_global_pos - PAGE;
          int got = extract_secret_in_page(page, pos_in_curr, out, MAX_SECRET - 1);
          if (got > 0) {
            printf("%s\n", out);
            exit(0);
          }
        }
      }
    }

    // advance prev_page pointer
    prev_page = page;
  }

  // not found
  exit(0);
}
