#include <stdbool.h>

#define CANNON_W (13)
#define CANNON_H (8)
#define CRAB_W (11)
#define CRAB_H (8)
#define LAZER_W (3)
#define LAZER_H (7)
#define MISSILE_W (2)
#define MISSILE_H (5)
#define OCTO_W (12)
#define OCTO_H (8)
#define SQUID_W (8)
#define SQUID_H (8)
#define UFO_W (16)
#define UFO_H (7)
#define SCORE_W (36)
#define SCORE_H (7)
#define DIGIT_W (6)
#define DIGIT_H (7)
#define LARGE_DIGIT_W (16)
#define LARGE_DIGIT_H (23)
#define LARGE_DIGIT_GAP (3)
#define WIN_W (141)
#define WIN_H (23)
#define WIN_X (88)
#define WIN_Y (74)
#define LOSE_W (155)
#define LOSE_H (23)
#define LOSE_X (81)
#define LOSE_Y (74)
#define LARGE_SCORE_W (103)
#define LARGE_SCORE_H (23)
#define LARGE_SCORE_X (79)
#define LARGE_SCORE_Y (104)
#define LARGE_SCORE_DIGIT_X (186)
#define LARGE_SCORE_DIGIT_Y (104)
#define TRY_AGAIN_W (167)
#define TRY_AGAIN_H (9)
#define TRY_AGAIN_X (76)
#define TRY_AGAIN_Y (164)
#define PRESENTS_W (75)
#define PRESENTS_H (19)
#define PRESENTS_X (123)
#define PRESENTS_Y (20)
#define LOGO_W (139)
#define LOGO_H (53)
#define LOGO_X (90)
#define LOGO_Y (74)
#define PLAY_W (136)
#define PLAY_H (9)
#define PLAY_X (93)
#define PLAY_Y (171)

typedef struct {
  int32_t x;
  int32_t y;
  int32_t w;
  int32_t h;
  uint8_t *pixels;
  uint8_t *primary;
  uint8_t *secondary;
  int32_t anim_count;
} sprite;

uint8_t *cannon1_pixels;
uint8_t *cannon2_pixels;
uint8_t *crab1_pixels;
uint8_t *crab2_pixels;
uint8_t *lazer_pixels;
uint8_t *missile_pixels;
uint8_t *octo1_pixels;
uint8_t *octo2_pixels;
uint8_t *squid1_pixels;
uint8_t *squid2_pixels;
uint8_t *ufo_pixels;
uint8_t *score_pixels;
uint8_t **digits_pixels;
uint8_t *win_pixels;
uint8_t *lose_pixels;
uint8_t *large_score_pixels;
uint8_t *try_again_pixels;
uint8_t **large_digits_pixels;
uint8_t *presents_pixels;
uint8_t *logo_pixels;
uint8_t *play_pixels;
