/*
 * Copyright (C) 2017 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef CONFIG_MTK_FPSGO_FBT_GAME
void update_pwd_tbl(void);
int reduce_stall(int, int, int);
#else
static inline void update_pwd_tbl(void) { }
static inline int reduce_stall(int bv, int thres, int flag) { return -1; }
#endif
