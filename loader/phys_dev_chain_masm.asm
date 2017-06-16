;
; Copyright (c) 2017 The Khronos Group Inc.
; Copyright (c) 2017 Valve Corporation
; Copyright (c) 2017 LunarG, Inc.
;
; Licensed under the Apache License, Version 2.0 (the "License");
; you may not use this file except in compliance with the License.
; You may obtain a copy of the License at
;
;     http://www.apache.org/licenses/LICENSE-2.0
;
; Unless required by applicable law or agreed to in writing, software
; distributed under the License is distributed on an "AS IS" BASIS,
; WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
; See the License for the specific language governing permissions and
; limitations under the License.
;
; Author: Lenny Komow <lenny@lunarg.com>
;

; This code is used to pass on physical device extensions through the call chain. It must do this without creating a stack frame,
; because the actual parameters of the call are not known. Since the first parameter is known to be a VkPhysicalDevice, it can
; unwrap the physical device, overwriting the wrapped device, and then jump to the next function in the call chain

; PHYS_DEV_DISP_OFFSET is defined in codegen
INCLUDE gen_defines.asm

; 64-bit values and macro
IFDEF rax

PHYS_DEV_SIZE           equ 8
PHYS_DEV_UNWRAP_OFFSET  equ 16
PTR_SIZE                equ 8

PhysDevExtTramp macro num:req
public vkPhysDevExtTramp&num&
vkPhysDevExtTramp&num&:
    mov     rax, qword ptr [rcx]                            ; Dereference the wrapped VkPhysicalDevice to get the dispatch table in rax
    mov     rcx, qword ptr [rcx + PHYS_DEV_UNWRAP_OFFSET]   ; Load the unwrapped VkPhysicalDevice into rcx
    jmp     qword ptr [rax + (PHYS_DEV_DISP_OFFSET + (PTR_SIZE * num))] ; Jump to the next function in the chain, preserving the args in other registers
endm

; 32-bit values and macro
ELSE

PHYS_DEV_SIZE           equ 4
PHYS_DEV_UNWRAP_OFFSET  equ 8
PTR_SIZE                equ 4

PhysDevExtTramp macro num
public _vkPhysDevExtTramp&num&@4
_vkPhysDevExtTramp&num&@4:
    mov     eax, dword ptr [esp + PHYS_DEV_SIZE]            ; Load the wrapped VkPhysicalDevice into eax
    mov     ecx, [eax + PHYS_DEV_UNWRAP_OFFSET]             ; Load the unwrapped VkPhysicalDevice into ecx
    mov     [esp + PHYS_DEV_SIZE], ecx                      ; Overwrite the wrapped VkPhysicalDevice with the unwrapped one (on the stack)
    mov     eax, [eax]                                      ; Dereference the wrapped VkPhysicalDevice to get the dispatch table in eax
    jmp     dword ptr [eax + (PHYS_DEV_DISP_OFFSET + (PTR_SIZE * num))] ; Jump to the next function in the chain, preserving the args on the stack
endm

; This is also needed for 32-bit only
.model flat

ENDIF

.code

    PhysDevExtTramp 0
    PhysDevExtTramp 1
    PhysDevExtTramp 2
    PhysDevExtTramp 3
    PhysDevExtTramp 4
    PhysDevExtTramp 5
    PhysDevExtTramp 6
    PhysDevExtTramp 7
    PhysDevExtTramp 8
    PhysDevExtTramp 9
    PhysDevExtTramp 10
    PhysDevExtTramp 11
    PhysDevExtTramp 12
    PhysDevExtTramp 13
    PhysDevExtTramp 14
    PhysDevExtTramp 15
    PhysDevExtTramp 16
    PhysDevExtTramp 17
    PhysDevExtTramp 18
    PhysDevExtTramp 19
    PhysDevExtTramp 20
    PhysDevExtTramp 21
    PhysDevExtTramp 22
    PhysDevExtTramp 23
    PhysDevExtTramp 24
    PhysDevExtTramp 25
    PhysDevExtTramp 26
    PhysDevExtTramp 27
    PhysDevExtTramp 28
    PhysDevExtTramp 29
    PhysDevExtTramp 30
    PhysDevExtTramp 31
    PhysDevExtTramp 32
    PhysDevExtTramp 33
    PhysDevExtTramp 34
    PhysDevExtTramp 35
    PhysDevExtTramp 36
    PhysDevExtTramp 37
    PhysDevExtTramp 38
    PhysDevExtTramp 39
    PhysDevExtTramp 40
    PhysDevExtTramp 41
    PhysDevExtTramp 42
    PhysDevExtTramp 43
    PhysDevExtTramp 44
    PhysDevExtTramp 45
    PhysDevExtTramp 46
    PhysDevExtTramp 47
    PhysDevExtTramp 48
    PhysDevExtTramp 49
    PhysDevExtTramp 50
    PhysDevExtTramp 51
    PhysDevExtTramp 52
    PhysDevExtTramp 53
    PhysDevExtTramp 54
    PhysDevExtTramp 55
    PhysDevExtTramp 56
    PhysDevExtTramp 57
    PhysDevExtTramp 58
    PhysDevExtTramp 59
    PhysDevExtTramp 60
    PhysDevExtTramp 61
    PhysDevExtTramp 62
    PhysDevExtTramp 63
    PhysDevExtTramp 64
    PhysDevExtTramp 65
    PhysDevExtTramp 66
    PhysDevExtTramp 67
    PhysDevExtTramp 68
    PhysDevExtTramp 69
    PhysDevExtTramp 70
    PhysDevExtTramp 71
    PhysDevExtTramp 72
    PhysDevExtTramp 73
    PhysDevExtTramp 74
    PhysDevExtTramp 75
    PhysDevExtTramp 76
    PhysDevExtTramp 77
    PhysDevExtTramp 78
    PhysDevExtTramp 79
    PhysDevExtTramp 80
    PhysDevExtTramp 81
    PhysDevExtTramp 82
    PhysDevExtTramp 83
    PhysDevExtTramp 84
    PhysDevExtTramp 85
    PhysDevExtTramp 86
    PhysDevExtTramp 87
    PhysDevExtTramp 88
    PhysDevExtTramp 89
    PhysDevExtTramp 90
    PhysDevExtTramp 91
    PhysDevExtTramp 92
    PhysDevExtTramp 93
    PhysDevExtTramp 94
    PhysDevExtTramp 95
    PhysDevExtTramp 96
    PhysDevExtTramp 97
    PhysDevExtTramp 98
    PhysDevExtTramp 99
    PhysDevExtTramp 100
    PhysDevExtTramp 101
    PhysDevExtTramp 102
    PhysDevExtTramp 103
    PhysDevExtTramp 104
    PhysDevExtTramp 105
    PhysDevExtTramp 106
    PhysDevExtTramp 107
    PhysDevExtTramp 108
    PhysDevExtTramp 109
    PhysDevExtTramp 110
    PhysDevExtTramp 111
    PhysDevExtTramp 112
    PhysDevExtTramp 113
    PhysDevExtTramp 114
    PhysDevExtTramp 115
    PhysDevExtTramp 116
    PhysDevExtTramp 117
    PhysDevExtTramp 118
    PhysDevExtTramp 119
    PhysDevExtTramp 120
    PhysDevExtTramp 121
    PhysDevExtTramp 122
    PhysDevExtTramp 123
    PhysDevExtTramp 124
    PhysDevExtTramp 125
    PhysDevExtTramp 126
    PhysDevExtTramp 127
    PhysDevExtTramp 128
    PhysDevExtTramp 129
    PhysDevExtTramp 130
    PhysDevExtTramp 131
    PhysDevExtTramp 132
    PhysDevExtTramp 133
    PhysDevExtTramp 134
    PhysDevExtTramp 135
    PhysDevExtTramp 136
    PhysDevExtTramp 137
    PhysDevExtTramp 138
    PhysDevExtTramp 139
    PhysDevExtTramp 140
    PhysDevExtTramp 141
    PhysDevExtTramp 142
    PhysDevExtTramp 143
    PhysDevExtTramp 144
    PhysDevExtTramp 145
    PhysDevExtTramp 146
    PhysDevExtTramp 147
    PhysDevExtTramp 148
    PhysDevExtTramp 149
    PhysDevExtTramp 150
    PhysDevExtTramp 151
    PhysDevExtTramp 152
    PhysDevExtTramp 153
    PhysDevExtTramp 154
    PhysDevExtTramp 155
    PhysDevExtTramp 156
    PhysDevExtTramp 157
    PhysDevExtTramp 158
    PhysDevExtTramp 159
    PhysDevExtTramp 160
    PhysDevExtTramp 161
    PhysDevExtTramp 162
    PhysDevExtTramp 163
    PhysDevExtTramp 164
    PhysDevExtTramp 165
    PhysDevExtTramp 166
    PhysDevExtTramp 167
    PhysDevExtTramp 168
    PhysDevExtTramp 169
    PhysDevExtTramp 170
    PhysDevExtTramp 171
    PhysDevExtTramp 172
    PhysDevExtTramp 173
    PhysDevExtTramp 174
    PhysDevExtTramp 175
    PhysDevExtTramp 176
    PhysDevExtTramp 177
    PhysDevExtTramp 178
    PhysDevExtTramp 179
    PhysDevExtTramp 180
    PhysDevExtTramp 181
    PhysDevExtTramp 182
    PhysDevExtTramp 183
    PhysDevExtTramp 184
    PhysDevExtTramp 185
    PhysDevExtTramp 186
    PhysDevExtTramp 187
    PhysDevExtTramp 188
    PhysDevExtTramp 189
    PhysDevExtTramp 190
    PhysDevExtTramp 191
    PhysDevExtTramp 192
    PhysDevExtTramp 193
    PhysDevExtTramp 194
    PhysDevExtTramp 195
    PhysDevExtTramp 196
    PhysDevExtTramp 197
    PhysDevExtTramp 198
    PhysDevExtTramp 199
    PhysDevExtTramp 200
    PhysDevExtTramp 201
    PhysDevExtTramp 202
    PhysDevExtTramp 203
    PhysDevExtTramp 204
    PhysDevExtTramp 205
    PhysDevExtTramp 206
    PhysDevExtTramp 207
    PhysDevExtTramp 208
    PhysDevExtTramp 209
    PhysDevExtTramp 210
    PhysDevExtTramp 211
    PhysDevExtTramp 212
    PhysDevExtTramp 213
    PhysDevExtTramp 214
    PhysDevExtTramp 215
    PhysDevExtTramp 216
    PhysDevExtTramp 217
    PhysDevExtTramp 218
    PhysDevExtTramp 219
    PhysDevExtTramp 220
    PhysDevExtTramp 221
    PhysDevExtTramp 222
    PhysDevExtTramp 223
    PhysDevExtTramp 224
    PhysDevExtTramp 225
    PhysDevExtTramp 226
    PhysDevExtTramp 227
    PhysDevExtTramp 228
    PhysDevExtTramp 229
    PhysDevExtTramp 230
    PhysDevExtTramp 231
    PhysDevExtTramp 232
    PhysDevExtTramp 233
    PhysDevExtTramp 234
    PhysDevExtTramp 235
    PhysDevExtTramp 236
    PhysDevExtTramp 237
    PhysDevExtTramp 238
    PhysDevExtTramp 239
    PhysDevExtTramp 240
    PhysDevExtTramp 241
    PhysDevExtTramp 242
    PhysDevExtTramp 243
    PhysDevExtTramp 244
    PhysDevExtTramp 245
    PhysDevExtTramp 246
    PhysDevExtTramp 247
    PhysDevExtTramp 248
    PhysDevExtTramp 249

end
