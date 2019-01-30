#include "output.h"

int term_color_mode = -1;
int term_num_colors = 2;

/* Set term_num_colors and term_color_mode or
 * return -1.
 */
int set_term_color_mode(void){
  char *cmd = "tput colors";

  char *buf = malloc(_BUFSIZE);
  FILE *fp;

  // Set fallback values
  term_color_mode = COLOR_SW_MODE;
  term_num_colors = 2;

  if ((fp = popen(cmd, "r")) == NULL) {
    fprintf(stderr, "Error opening pipe!\n");
    free(buf);
    return -1;
  }

  if (fgets(buf, _BUFSIZE, fp) != NULL) {

    // Remove disturbing newline
    char *pnewline =  strchr(buf, '\n');
    if( pnewline ){ *pnewline = '\0'; }

    char *endptr;
    int val = (int) strtol(buf, &endptr, 0);
    if( *endptr == '\0' ){ /* string was valid */
      term_num_colors = val;
      if (val >= 256){ /* Assume that it also supports rgb mode
                          ('\033[38;2;%d;%d;%dm')  */
        term_color_mode = COLOR_RGB_MODE;
      }else if (val >= 8){
        term_color_mode = COLOR_LIMITED_MODE;
      }else{
        term_color_mode = COLOR_SW_MODE;
      }
    }
    //fprintf(stderr, "OUTPUT: %s Colors: %d, Mode: %d\n", buf, term_num_colors, term_color_mode);
  }

  if(pclose(fp))  {
    fprintf(stderr, "Command not found or exited with error status\n");
    free(buf);
    return -1;
  }

  free(buf);
  return 0;
}

int sprintf_color(char *buf,
        int set_background,
        int r, int g, int b,
        const char *pextra_str)
{

    int consumed_chars = 0;
    if( term_color_mode == COLOR_RGB_MODE ){
        if( set_background > 1 ){
            // Also adapt foreground color on background for better readability
            if( r + g + b > 400 ){
                consumed_chars += sprintf(buf+consumed_chars, "\x1b[%d;2;%d;%d;%dm", 38, 0, 0, 0);
            }else{
                consumed_chars += sprintf(buf+consumed_chars, "\x1b[%d;2;%d;%d;%dm", 38, 240, 240, 240);
            }
        }
        consumed_chars +=  sprintf(buf+consumed_chars, "\x1b[%d;2;%d;%d;%dm%s",
                      set_background?48:38, r, g, b,
                      pextra_str?pextra_str:"");

    }else if( term_color_mode == COLOR_LIMITED_MODE ){
        int col = (r^g^b)%8; // one of the 8 colors
        int bright = (r+g+b > 400); // bright flag
        if( set_background > 1 ){
            /* Note: 0;-reset is required in some terminals. Otherwise,
             * the bold/bright state is not reset as expected. */
            if( bright==0 ){
                consumed_chars +=  sprintf(buf, "\x1b[30;1m"); // black
            }else{
                consumed_chars +=  sprintf(buf, "\x1b[0;37m"); // white
            }
        }
        consumed_chars +=  sprintf(buf+consumed_chars, "\x1b[%d%sm%s",
                col + (set_background?40:30),
                bright?";1":"",
                pextra_str?pextra_str:"");

    }else if(pextra_str){
        consumed_chars +=  sprintf(buf, "%s",
                pextra_str?pextra_str:"");

    }


    return consumed_chars;
}

/*
 * Explands buffer if less than 'required' characters left.
 */
void expand_buf_if_required(
        size_t *pbuf_len,  // Length of *ppbuf
        char **ppbuf,
        size_t already_used,  // <= *pbuf_len
        size_t required,
        size_t extra_padding  // reallocation to size buf_len-buf_used + required  + extra_padding
        ){
    if( *pbuf_len > already_used + required ) return;
    if( *pbuf_len < already_used ){
        fprintf(stderr, "(%s) Hey buf_used=%lu > %lu=buf_len\n", __FILE__, already_used, *pbuf_len);
        return;
    }
    size_t new_len = already_used + required + extra_padding;
    //fprintf(stderr, "Increase from %lu to %lu \n", *pbuf_len, new_len);
    *ppbuf = realloc(*ppbuf, new_len * sizeof(typeof(*pbuf_len)));
    *pbuf_len = new_len;
}

