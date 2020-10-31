#define EXPLOSION_LEN (17462)
#define MISSILE_SHOOT_LEN (6754)
#define INVADER_DEATH_LEN (8160)

int16_t *explosion_audio;
int16_t *missile_shoot_audio;
int16_t *invader_death_audio;

typedef struct {
  int32_t length;
  int16_t *audio;
} sound;
