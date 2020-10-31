/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by ST under BSD 3-Clause license,
 * the "License"; You may not use this file except in compliance with the
 * License. You may obtain a copy of the License at:
 *                        opensource.org/licenses/BSD-3-Clause
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "sound.h"
#include "sprite.h"

#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
/* USER CODE END Includes */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define SCREEN_START ((u8 *)0x20020000)
#define SCREEN_W (320)
#define SCREEN_H (200)
#define SCREEN_END ((u8 *)(SCREEN_START + SCREEN_W * SCREEN_H - 1))

#define MAX_CANNON_X (SCREEN_W - (CANNON_W + 1))
#define MAX_INVADER_WIDTH (max(max(CRAB_W, SQUID_W), OCTO_W))
#define MAX_LAZER_COUNT (16)
#define MAX_INVADER_SOUND_COUNT (4)

#define CANNON_SPEED (15)
#define INVADER_SPEED (10)
#define MISSILE_SPEED (10)
#define LAZER_SPEED (10)
#define INVADER_ROW_SIZE (5)
#define INVADER_COLUMN_SIZE (6)
#define TOTAL_INVADER_COUNT (INVADER_ROW_SIZE * INVADER_COLUMN_SIZE)
#define LAZER_TICK_DELAY (100)
#define MENU_DELAY (20)
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
#define min(a, b) (a < b ? a : b)

#define max(a, b) (a > b ? a : b)

#define clamp(x, lower, upper) (min(max(x, lower), upper))
/* USER CODE END PM */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef uint8_t u8;
typedef int8_t i8;
typedef uint16_t u16;
typedef int16_t i16;
typedef uint32_t u32;
typedef int32_t i32;

typedef enum { TITLE_SCREEN, IN_PROGRESS, PLAYER_WIN, PLAYER_LOSE } gamestate;

typedef struct {
  sprite sprites[INVADER_COLUMN_SIZE];
  bool dead[INVADER_COLUMN_SIZE];
  int32_t velocity;
  int32_t bounty;
  bool movedown;
  bool animate;
  i32 alive;
} invaderrow;

typedef struct {
  gamestate state;
  sprite hero;
  i32 hero_lives;
  i32 score;
  invaderrow invaders[INVADER_ROW_SIZE];
  i32 current_invader_row;
  i32 invaders_alive;
  sprite missile;
  sprite lazers[MAX_LAZER_COUNT];
  u32 last_lazer_tick;
  sound missile_shoot_sound;
  sound invader_death_sound;
  sound explosion_sound;
} game;
/* USER CODE END PTD */

/* Private variables ---------------------------------------------------------*/
I2S_HandleTypeDef hi2s3;
DMA_HandleTypeDef hdma_spi3_tx;

DMA_HandleTypeDef hdma_memtomem_dma2_stream0;
/* USER CODE BEGIN PV */
volatile bool refresh;
volatile bool moving_left;
volatile bool moving_right;
volatile bool shooting;
volatile bool cleared;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_I2S3_Init(void);
/* USER CODE BEGIN PFP */
void StartGame(game *g);
void BitBlit(sprite *s);
void ClearScreen();
void ProcessInvaders(game *g);
void PlaySound(sound *s);
bool HitTest(sprite *s1, sprite *s2);
void DisplayScore(i32 score);
void DisplayLives(i32 lives);
void UpdateGame(game *g);
void DisplayEndScore(i32 score, sprite *message);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void StartGame(game *g) {
  i32 i;
  i32 j;
  sprite *invader;

  g->hero.primary = cannon1_pixels;
  g->hero.secondary = cannon2_pixels;
  g->hero.pixels = g->hero.primary;
  g->hero.w = CANNON_W;
  g->hero.h = CANNON_H;

  g->hero.x = SCREEN_W / 2 - CANNON_W;
  g->hero.y = SCREEN_H - 2 * (CANNON_H + 1) - 3;
  g->hero.anim_count = 0;

  for (i = 0; i < INVADER_ROW_SIZE; ++i) {
    g->invaders[i].velocity = INVADER_SPEED;
    g->invaders[i].movedown = false;
    g->invaders[i].animate = false;
    g->invaders[i].alive = INVADER_COLUMN_SIZE;

    for (j = 0; j < INVADER_COLUMN_SIZE; ++j) {
      invader = &g->invaders[i].sprites[j];
      g->invaders[i].dead[j] = false;

      switch (i) {
      case 0:
        g->invaders[i].bounty = 30;
        invader->primary = squid1_pixels;
        invader->secondary = squid2_pixels;
        invader->w = SQUID_W;
        invader->h = SQUID_H;
        break;
      case 1:
      case 2:
        g->invaders[i].bounty = 20;
        invader->primary = crab1_pixels;
        invader->secondary = crab2_pixels;
        invader->w = CRAB_W;
        invader->h = CRAB_H;
        break;
      case 3:
      case 4:
        g->invaders[i].bounty = 10;
        invader->primary = octo1_pixels;
        invader->secondary = octo2_pixels;
        invader->w = OCTO_W;
        invader->h = OCTO_H;
        break;
      }

      invader->x = j * (MAX_INVADER_WIDTH + 10);
      invader->x += (MAX_INVADER_WIDTH - invader->w) / 2;
      invader->y = SCORE_H + 10 + i * (invader->h + 10);
      invader->pixels = invader->primary;
    }
  }

  g->missile.w = MISSILE_W;
  g->missile.h = MISSILE_H;
  g->missile.x = 0;
  g->missile.y = 0;
  g->missile.pixels = missile_pixels;

  for (i = 0; i < MAX_LAZER_COUNT; ++i) {
    g->lazers[i].w = LAZER_W;
    g->lazers[i].h = LAZER_H;
    g->lazers[i].x = 0;
    g->lazers[i].y = SCREEN_H + LAZER_H;
    g->lazers[i].pixels = lazer_pixels;
  }

  g->missile_shoot_sound.audio = missile_shoot_audio;
  g->missile_shoot_sound.length = MISSILE_SHOOT_LEN;
  g->invader_death_sound.audio = invader_death_audio;
  g->invader_death_sound.length = INVADER_DEATH_LEN;
  g->explosion_sound.audio = explosion_audio;
  g->explosion_sound.length = EXPLOSION_LEN;
  g->current_invader_row = 0;
  g->invaders_alive = TOTAL_INVADER_COUNT;
  g->hero_lives = 3;
  g->state = IN_PROGRESS;
  g->score = 0;
  g->last_lazer_tick = HAL_GetTick();
}

void BitBlit(sprite *s) {
  u32 *word_src;
  u32 *word_dst;
  u8 *src;
  u8 *dst;
  i32 i;
  i32 j;
  i32 word_width;
  i32 byte_width;
  i32 alignment_shift;

  src = s->pixels;
  dst = SCREEN_START + s->x + s->y * SCREEN_W;
  alignment_shift = (u16)s->x % 4;
  if (alignment_shift > s->w) {
    alignment_shift = s->w;
  }

  word_width = (s->w - alignment_shift) / 4;
  byte_width = (s->w - alignment_shift) % 4;

  for (j = 0; j < s->h; ++j) {
    for (i = 0; i < alignment_shift; ++i) {
      *dst++ = *src++;
    }

    word_src = (u32 *)src;
    word_dst = (u32 *)dst;

    for (i = 0; i < word_width; ++i) {
      *word_dst++ = *word_src++;
    }

    src = (u8 *)word_src;
    dst = (u8 *)word_dst;

    for (i = 0; i < byte_width; ++i) {
      *dst++ = *src++;
    }

    dst += SCREEN_W - s->w;
  }
}

void ClearScreen() {
  u32 *src;
  u32 *dst;
  i32 block_count;

  src = (u32 *)SCREEN_START;
  dst = src + 1;
  *src = 0;

  block_count = (SCREEN_W * SCREEN_H / 4) - 1;

  cleared = false;
  HAL_DMA_Start_IT(&hdma_memtomem_dma2_stream0, (u32)src, (u32)dst,
                   block_count);
  while (!cleared) {
  }
}

void ProcessInvaders(game *g) {
  i32 i;
  i32 v;
  i32 invader_index;
  invaderrow *invader_row;
  sprite *invader;

  invader_row = &g->invaders[INVADER_ROW_SIZE - (g->current_invader_row + 1)];
  v = invader_row->velocity;
  for (i = 0; i < INVADER_COLUMN_SIZE; ++i) {
    if (v > 0) {
      invader_index = i;
    } else {
      invader_index = INVADER_COLUMN_SIZE - (i + 1);
    }

    invader = &invader_row->sprites[invader_index];

    if (invader_row->animate) {
      invader->pixels = invader->secondary;
    } else {
      invader->pixels = invader->primary;
    }

    if (invader_row->movedown) {
      invader->y += INVADER_SPEED;

      if (!invader_row->dead[invader_index] &&
          invader->y + invader->h > g->hero.y - g->hero.h) {
        g->state = PLAYER_LOSE;
      }

      if (i == INVADER_COLUMN_SIZE - 1) {
        invader_row->movedown = false;
      }

      continue;
    }

    invader->x += v;

    if (invader->x + invader_row->velocity >
            SCREEN_W - (MAX_INVADER_WIDTH + 1) ||
        invader->x + invader_row->velocity < 0) {
      v *= -1;
      invader_row->velocity = v;
      v *= -1;
      invader_row->movedown = true;
    }
  }

  invader_row->animate = !invader_row->animate;
}

void PlaySound(sound *s) {
  HAL_I2S_Transmit_DMA(&hi2s3, (u16 *)s->audio, s->length);
}

bool HitTest(sprite *s1, sprite *s2) {
  i32 edges1[4];
  i32 edges2[4];

  edges1[0] = s1->x;
  edges1[1] = s1->y;
  edges1[2] = s1->x + s1->w;
  edges1[3] = s1->y + s1->h;
  edges2[0] = s2->x;
  edges2[1] = s2->y;
  edges2[2] = s2->x + s2->w;
  edges2[3] = s2->y + s2->h;

  return edges1[0] < edges2[2] && edges1[2] > edges2[0] &&
         edges1[1] < edges2[3] && edges1[3] > edges2[1];
}

void DisplayScore(i32 score) {
  sprite score_sprite;
  i32 number[3];
  sprite digits[3];
  i32 start;
  i32 i;

  score_sprite.pixels = score_pixels;
  score_sprite.w = SCORE_W;
  score_sprite.h = SCORE_H;
  score_sprite.x = 1;
  score_sprite.y = 1;
  BitBlit(&score_sprite);

  number[0] = (score / 100) % 10;
  number[1] = (score / 10) % 10;
  number[2] = score % 10;

  if (score > 99) {
    start = 0;
  } else if (score > 9) {
    start = 1;
  } else {
    start = 2;
  }

  for (i = start; i < 3; ++i) {
    digits[i].pixels = digits_pixels[number[i]];
    digits[i].x = (score_sprite.x + SCORE_W + 5) + (i - start) * (DIGIT_W + 1);
    digits[i].y = score_sprite.y;
    digits[i].w = DIGIT_W;
    digits[i].h = DIGIT_H;
    BitBlit(&digits[i]);
  }
}

void DisplayLives(i32 lives) {
  i32 i;
  sprite cannon;

  cannon.w = CANNON_W;
  cannon.h = CANNON_H;
  cannon.y = SCREEN_H - (CANNON_H + 1);
  cannon.pixels = cannon1_pixels;

  for (i = 0; i < lives; ++i) {
    cannon.x = i * (CANNON_W + 3);
    BitBlit(&cannon);
  }
}

void UpdateGame(game *g) {
  i32 i;
  i32 j;
  i32 tick;
  i32 random_invader_column;
  sprite *lazer;
  sprite *random_invader;
  invaderrow *invader_row;
  bool invader_hit;

  tick = HAL_GetTick();
  ProcessInvaders(g);

  invader_hit = false;
  if (g->missile.y - MISSILE_SPEED > 0) {
    g->missile.y -= MISSILE_SPEED;
    for (i = 0; i < INVADER_ROW_SIZE && !invader_hit; ++i) {
      invader_row = &g->invaders[i];
      for (j = 0; j < INVADER_COLUMN_SIZE && !invader_hit; ++j) {
        if (HitTest(&g->missile, &invader_row->sprites[j]) &&
            !invader_row->dead[j]) {
          invader_hit = true;
          invader_row->dead[j] = true;
          invader_row->alive -= 1;
          g->invaders_alive -= 1;
          if (g->invaders_alive == 0) {
            g->state = PLAYER_WIN;
          }

          PlaySound(&g->invader_death_sound);
          g->missile.y = 0;
          g->score += g->invaders[i].bounty;
        }
      }
    }

    if (!invader_hit) {
      BitBlit(&g->missile);
    }

    if (shooting) {
      shooting = false;
    }
  } else if (shooting) {
    g->missile.x = g->hero.x + (g->hero.w - g->missile.w) / 2;
    g->missile.y = g->hero.y - g->hero.h;

    PlaySound(&g->missile_shoot_sound);
  }

  for (i = 0; i < MAX_LAZER_COUNT; ++i) {
    lazer = &g->lazers[i];
    if (lazer->y + lazer->h + LAZER_SPEED < SCREEN_H) {
      lazer->y += LAZER_SPEED;
      if (HitTest(lazer, &g->hero)) {
        g->hero_lives -= 1;
        if (g->hero_lives < 0) {
          g->state = PLAYER_LOSE;
        }
        lazer->y = SCREEN_H;

        g->hero.anim_count = 1;
        PlaySound(&g->explosion_sound);
      } else {
        BitBlit(lazer);
      }
    } else if (tick - g->last_lazer_tick > LAZER_TICK_DELAY) {
      g->last_lazer_tick = tick;

      do {
        random_invader_column = rand() % INVADER_COLUMN_SIZE;
        random_invader = &g->invaders[0].sprites[random_invader_column];
      } while (g->invaders[0].dead[random_invader_column]);

      lazer->x = random_invader->x + (random_invader->w - lazer->w) / 2;
      lazer->y = random_invader->y + random_invader->h;
    }
  }

  switch (g->hero.anim_count) {
  case 0:
    g->hero.pixels = g->hero.primary;
    break;
  case 1:
  case 3:
  case 5:
    g->hero.pixels = g->hero.secondary;
    ++g->hero.anim_count;
    break;
  case 2:
  case 4:
    g->hero.pixels = g->hero.primary;
    ++g->hero.anim_count;
    break;
  default:
    g->hero.pixels = g->hero.primary;
    g->hero.anim_count = 0;
  }

  BitBlit(&g->hero);

  for (i = 0; i < INVADER_ROW_SIZE; ++i) {
    for (j = 0; j < INVADER_COLUMN_SIZE; ++j) {
      if (!g->invaders[i].dead[j]) {
        BitBlit(&g->invaders[i].sprites[j]);
      }
    }
  }

  ++g->current_invader_row;
  if (g->current_invader_row == INVADER_ROW_SIZE) {
    g->current_invader_row = 0;
  }

  DisplayScore(g->score);
  DisplayLives(g->hero_lives);
}

void DisplayEndScore(i32 score, sprite *message) {
  sprite score_sprite;
  sprite try_again_sprite;
  i32 number[3];
  sprite digits[3];
  i32 i;

  BitBlit(message);

  score_sprite.pixels = large_score_pixels;
  score_sprite.w = LARGE_SCORE_W;
  score_sprite.h = LARGE_SCORE_H;
  score_sprite.x = LARGE_SCORE_X;
  score_sprite.y = LARGE_SCORE_Y;
  BitBlit(&score_sprite);

  number[0] = (score / 100) % 10;
  number[1] = (score / 10) % 10;
  number[2] = score % 10;

  for (i = 0; i < 3; ++i) {
    digits[i].pixels = large_digits_pixels[number[i]];
    digits[i].w = LARGE_DIGIT_W;
    digits[i].h = LARGE_DIGIT_H;
    digits[i].x = LARGE_SCORE_DIGIT_X + i * (LARGE_DIGIT_W + LARGE_DIGIT_GAP);
    digits[i].y = LARGE_SCORE_DIGIT_Y;
    BitBlit(&digits[i]);
  }

  try_again_sprite.pixels = try_again_pixels;
  try_again_sprite.w = TRY_AGAIN_W;
  try_again_sprite.h = TRY_AGAIN_H;
  try_again_sprite.x = TRY_AGAIN_X;
  try_again_sprite.y = TRY_AGAIN_Y;
  BitBlit(&try_again_sprite);
}
/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {
  /* USER CODE BEGIN 1 */
  game g;
  sprite presents_sprite;
  sprite logo_sprite;
  sprite play_sprite;
  sprite lose_sprite;
  sprite win_sprite;
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick.
   */
  HAL_Init();

  /* USER CODE BEGIN Init */
  moving_left = false;
  moving_right = false;
  shooting = false;
  refresh = true;
  srand(time(0));
  g.state = TITLE_SCREEN;
  presents_sprite.w = PRESENTS_W;
  presents_sprite.h = PRESENTS_H;
  presents_sprite.x = PRESENTS_X;
  presents_sprite.y = PRESENTS_Y;
  presents_sprite.pixels = presents_pixels;
  logo_sprite.w = LOGO_W;
  logo_sprite.h = LOGO_H;
  logo_sprite.x = LOGO_X;
  logo_sprite.y = LOGO_Y;
  logo_sprite.pixels = logo_pixels;
  play_sprite.w = PLAY_W;
  play_sprite.h = PLAY_H;
  play_sprite.x = PLAY_X;
  play_sprite.y = PLAY_Y;
  play_sprite.pixels = play_pixels;
  lose_sprite.w = LOSE_W;
  lose_sprite.h = LOSE_H;
  lose_sprite.x = LOSE_X;
  lose_sprite.y = LOSE_Y;
  lose_sprite.pixels = lose_pixels;
  win_sprite.w = WIN_W;
  win_sprite.h = WIN_H;
  win_sprite.x = WIN_X;
  win_sprite.y = WIN_Y;
  win_sprite.pixels = win_pixels;
  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_I2S3_Init();
  /* USER CODE BEGIN 2 */
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1) {
    if (g.state == TITLE_SCREEN) {
      BitBlit(&presents_sprite);
      BitBlit(&logo_sprite);
      BitBlit(&play_sprite);

      while (!shooting) {
        HAL_Delay(MENU_DELAY);
      }

      shooting = false;
      StartGame(&g);
    }

    while (g.state == IN_PROGRESS) {
      if (!refresh) {
        continue;
      }
      /* USER CODE END WHILE */

      /* USER CODE BEGIN 3 */
      ClearScreen();

      if (moving_left) {
        if (g.hero.x - CANNON_SPEED > 0) {
          g.hero.x -= CANNON_SPEED;
        } else {
          g.hero.x = 0;
        }
      }

      if (moving_right) {
        if (g.hero.x + CANNON_SPEED < MAX_CANNON_X) {
          g.hero.x += CANNON_SPEED;
        } else {
          g.hero.x = MAX_CANNON_X;
        }
      }

      UpdateGame(&g);

      refresh = false;
    }

    ClearScreen();

    if (g.state == PLAYER_LOSE) {
      DisplayEndScore(g.score, &lose_sprite);
    }

    if (g.state == PLAYER_WIN) {
      DisplayEndScore(g.score, &win_sprite);
    }

    HAL_Delay(100);

    while (!shooting) {
      HAL_Delay(MENU_DELAY);
    }

    shooting = false;
    StartGame(&g);
  }
  /* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

  /** Macro to configure the PLL multiplication factor
   */
  __HAL_RCC_PLL_PLLM_CONFIG(16);
  /** Macro to configure the PLL clock source
   */
  __HAL_RCC_PLL_PLLSOURCE_CONFIG(RCC_PLLSOURCE_HSI);
  /** Configure the main internal regulator output voltage
   */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  /** Initializes the CPU, AHB and APB busses clocks
   */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB busses clocks
   */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK) {
    Error_Handler();
  }
  PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_I2S;
  PeriphClkInitStruct.PLLI2S.PLLI2SN = 192;
  PeriphClkInitStruct.PLLI2S.PLLI2SM = 16;
  PeriphClkInitStruct.PLLI2S.PLLI2SR = 2;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK) {
    Error_Handler();
  }
}

/**
 * @brief I2S3 Initialization Function
 * @param None
 * @retval None
 */
static void MX_I2S3_Init(void) {

  /* USER CODE BEGIN I2S3_Init 0 */

  /* USER CODE END I2S3_Init 0 */

  /* USER CODE BEGIN I2S3_Init 1 */

  /* USER CODE END I2S3_Init 1 */
  hi2s3.Instance = SPI3;
  hi2s3.Init.Mode = I2S_MODE_MASTER_TX;
  hi2s3.Init.Standard = I2S_STANDARD_PHILIPS;
  hi2s3.Init.DataFormat = I2S_DATAFORMAT_16B;
  hi2s3.Init.MCLKOutput = I2S_MCLKOUTPUT_DISABLE;
  hi2s3.Init.AudioFreq = I2S_AUDIOFREQ_8K;
  hi2s3.Init.CPOL = I2S_CPOL_LOW;
  hi2s3.Init.ClockSource = I2S_CLOCK_PLL;
  hi2s3.Init.FullDuplexMode = I2S_FULLDUPLEXMODE_DISABLE;
  if (HAL_I2S_Init(&hi2s3) != HAL_OK) {
    Error_Handler();
  }
  /* USER CODE BEGIN I2S3_Init 2 */

  /* USER CODE END I2S3_Init 2 */
}

/**
 * Enable DMA controller clock
 * Configure DMA for memory to memory transfers
 *   hdma_memtomem_dma2_stream0
 */
static void MX_DMA_Init(void) {

  /* DMA controller clock enable */
  __HAL_RCC_DMA2_CLK_ENABLE();
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* Configure DMA request hdma_memtomem_dma2_stream0 on DMA2_Stream0 */
  hdma_memtomem_dma2_stream0.Instance = DMA2_Stream0;
  hdma_memtomem_dma2_stream0.Init.Channel = DMA_CHANNEL_0;
  hdma_memtomem_dma2_stream0.Init.Direction = DMA_MEMORY_TO_MEMORY;
  hdma_memtomem_dma2_stream0.Init.PeriphInc = DMA_PINC_ENABLE;
  hdma_memtomem_dma2_stream0.Init.MemInc = DMA_MINC_ENABLE;
  hdma_memtomem_dma2_stream0.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
  hdma_memtomem_dma2_stream0.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
  hdma_memtomem_dma2_stream0.Init.Mode = DMA_NORMAL;
  hdma_memtomem_dma2_stream0.Init.Priority = DMA_PRIORITY_LOW;
  hdma_memtomem_dma2_stream0.Init.FIFOMode = DMA_FIFOMODE_ENABLE;
  hdma_memtomem_dma2_stream0.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_FULL;
  hdma_memtomem_dma2_stream0.Init.MemBurst = DMA_MBURST_SINGLE;
  hdma_memtomem_dma2_stream0.Init.PeriphBurst = DMA_PBURST_SINGLE;
  if (HAL_DMA_Init(&hdma_memtomem_dma2_stream0) != HAL_OK) {
    Error_Handler();
  }

  /* DMA interrupt init */
  /* DMA1_Stream5_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream5_IRQn);
  /* DMA2_Stream0_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);
}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void) {
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin : PA0 */
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PD9 PD10 */
  GPIO_InitStruct.Pin = GPIO_PIN_9 | GPIO_PIN_10;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pin : PB4 */
  GPIO_InitStruct.Pin = GPIO_PIN_4;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);

  HAL_NVIC_SetPriority(EXTI4_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI4_IRQn);

  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}

/* USER CODE BEGIN 4 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
  switch (GPIO_Pin) {
  case GPIO_PIN_4:
    refresh = true;
    break;
  case GPIO_PIN_10:
    moving_left = !moving_left;
    break;
  case GPIO_PIN_9:
    moving_right = !moving_right;
    break;
  case GPIO_PIN_0:
    shooting = true;
    break;
  default:
    break;
  }
}
/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */

  /* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line) {
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line
     number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line)
   */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
