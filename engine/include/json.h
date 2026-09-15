/**
 * json.h
 * Descrição: Utilitários para serialização de TADs em JSON com buffer dinâmico.
 */
#ifndef JSON_H
#define JSON_H

#include "obra.h"
#include "resultado.h"

typedef struct json_buffer_t JsonBuffer;

JsonBuffer* json_buffer_criar(size_t capacidade_inicial);
void        json_buffer_liberar(JsonBuffer* jb);
const char* json_buffer_obter_texto(const JsonBuffer* jb);
size_t      json_buffer_tamanho(const JsonBuffer* jb);

void json_serializar_obra(JsonBuffer* jb, const Obra* o);
void json_serializar_resultado(JsonBuffer* jb, const Resultado* r,
                               const char* termo, const char* tipo,
                               const char* estrutura);

#endif