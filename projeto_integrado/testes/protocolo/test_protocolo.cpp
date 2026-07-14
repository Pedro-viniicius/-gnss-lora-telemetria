// test_protocolo.cpp
// -----------------------------------------------------------------------------
// Testes de host (g++) para a biblioteca protocolo_comunicacao. Compila os
// mesmos .cpp usados no firmware, garantindo que a logica de CRC, COBS e
// (de)serializacao seja identica em todas as placas.
//
// Uso: make && ./test_protocolo
// -----------------------------------------------------------------------------
#include <cstdio>
#include <cstring>
#include <cstdint>

#include "protocolo.h"

using namespace protocolo;

static int g_total = 0;
static int g_falhas = 0;

#define VERIFICAR(condicao, descricao)                               \
  do {                                                               \
    ++g_total;                                                       \
    if (condicao) {                                                  \
      std::printf("  [PASS] %s\n", descricao);                       \
    } else {                                                         \
      std::printf("  [FALHA] %s  (linha %d)\n", descricao, __LINE__);\
      ++g_falhas;                                                    \
    }                                                                \
  } while (0)

// --- Helpers ----------------------------------------------------------------
static Pacote fazerPacoteTelemetria() {
  TelemetriaGnss t;
  t.uptime_ms = 123456;
  t.latitude_1e7 = -211234567;    // ~ -21.1234567 graus
  t.longitude_1e7 = -451234567;   // ~ -45.1234567 graus
  t.altitude_mm = 987654;         // 987,654 m
  t.tipo_fix = FIX_RTK_FIXED;
  t.satelites = 24;
  t.precisao_horizontal_mm = 14;
  t.ultimo_comando_teclado = '7';
  t.flags = GNSS_FLAG_TEM_ALTITUDE | GNSS_FLAG_TEM_PRECISAO;

  Pacote pkt;
  montarPacoteTelemetria(pkt, ID_CONTROLADOR, 4242, t);
  return pkt;
}

// --- Testes -----------------------------------------------------------------
static void testeCrcVetoresConhecidos() {
  std::printf("[CRC16-CCITT] vetores conhecidos\n");
  const uint8_t msg[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};
  uint16_t crc = calcularCrc16(msg, sizeof(msg));
  VERIFICAR(crc == 0x29B1, "\"123456789\" => 0x29B1");

  const uint8_t vazio[1] = {0};
  VERIFICAR(calcularCrc16(vazio, 0) == 0xFFFF, "entrada vazia => 0xFFFF (init)");

  const uint8_t a[] = {'A'};
  // Recalculo incremental deve bater com o de bloco.
  uint16_t inc = atualizarCrc16(0xFFFF, 'A');
  VERIFICAR(inc == calcularCrc16(a, 1), "incremental == bloco para 1 byte");
}

static void testeCobsRoundTrip() {
  std::printf("[COBS] round-trip\n");
  const uint8_t casos[][8] = {
      {0x11, 0x22, 0x00, 0x33},          // com zero no meio
      {0x00, 0x00, 0x00, 0x00},          // so zeros
      {0x01, 0x02, 0x03, 0x04},          // sem zeros
  };
  const size_t tamanhos[] = {4, 4, 4};

  for (int i = 0; i < 3; ++i) {
    uint8_t cod[32];
    uint8_t dec[32];
    size_t nc = cobsEncode(casos[i], tamanhos[i], cod, sizeof(cod));
    VERIFICAR(nc > 0, "cobsEncode retornou tamanho > 0");
    // Nenhum 0x00 deve aparecer no quadro codificado.
    bool temZero = false;
    for (size_t k = 0; k < nc; ++k) {
      if (cod[k] == 0) temZero = true;
    }
    VERIFICAR(!temZero, "quadro COBS nao contem 0x00 interno");
    size_t nd = cobsDecode(cod, nc, dec, sizeof(dec));
    VERIFICAR(nd == tamanhos[i] && std::memcmp(dec, casos[i], nd) == 0,
              "cobsDecode(cobsEncode(x)) == x");
  }

  // Bloco longo (>254 bytes sem zero) exercita a troca de bloco 0xFF.
  uint8_t longo[300];
  for (int i = 0; i < 300; ++i) longo[i] = (uint8_t)(1 + (i % 200));
  uint8_t cod[512];
  uint8_t dec[512];
  size_t nc = cobsEncode(longo, sizeof(longo), cod, sizeof(cod));
  size_t nd = cobsDecode(cod, nc, dec, sizeof(dec));
  VERIFICAR(nd == sizeof(longo) && std::memcmp(dec, longo, nd) == 0,
            "COBS round-trip de bloco longo (300 bytes)");

  // Capacidade insuficiente deve falhar com 0 (sem estouro).
  uint8_t pequeno[2];
  size_t nf = cobsEncode(longo, sizeof(longo), pequeno, sizeof(pequeno));
  VERIFICAR(nf == 0, "cobsEncode com destino pequeno retorna 0");
}

static void testeEncodeDecodeRoundTrip() {
  std::printf("[PACOTE] round-trip encode/decode\n");
  Pacote pkt = fazerPacoteTelemetria();
  uint8_t buf[TAMANHO_MAXIMO_PACOTE];
  size_t n = codificarPacote(pkt, buf, sizeof(buf));
  VERIFICAR(n == TAMANHO_CABECALHO + TAMANHO_PAYLOAD_TELEMETRIA_GNSS + TAMANHO_CRC,
            "tamanho total do pacote correto");
  VERIFICAR(buf[0] == 0xAA && buf[1] == 0x55, "bytes magicos presentes");

  Pacote dec;
  ResultadoDecodificacao r = decodificarPacote(buf, n, dec, true);
  VERIFICAR(r == DECODE_OK, "decodifica pacote valido");
  VERIFICAR(dec.tipo == MSG_TELEMETRIA_GNSS, "tipo preservado");
  VERIFICAR(dec.sequencia == 4242, "sequencia preservada");
  VERIFICAR(dec.id_remetente == ID_CONTROLADOR, "id remetente preservado");
}

static void testeMinimoEMaximo() {
  std::printf("[PACOTE] minimo e maximo\n");
  // Pacote minimo: payload 0.
  Pacote pkt;
  pkt.versao = VERSAO_PROTOCOLO;
  pkt.tipo = MSG_HEARTBEAT;
  pkt.id_remetente = ID_CONTROLADOR;
  pkt.sequencia = 1;
  pkt.flags = 0;
  pkt.tamanho_payload = 0;
  uint8_t buf[TAMANHO_MAXIMO_PACOTE];
  size_t n = codificarPacote(pkt, buf, sizeof(buf));
  VERIFICAR(n == TAMANHO_MINIMO_PACOTE, "pacote minimo tem tamanho minimo");
  Pacote dec;
  VERIFICAR(decodificarPacote(buf, n, dec, true) == DECODE_OK,
            "decodifica pacote minimo");

  // Pacote maximo: payload = MAX_PAYLOAD.
  pkt.tipo = MSG_COMANDO;
  pkt.tamanho_payload = MAX_PAYLOAD;
  for (size_t i = 0; i < MAX_PAYLOAD; ++i) pkt.payload[i] = (uint8_t)(i + 1);
  n = codificarPacote(pkt, buf, sizeof(buf));
  VERIFICAR(n == TAMANHO_MAXIMO_PACOTE, "pacote maximo tem tamanho maximo");
  VERIFICAR(decodificarPacote(buf, n, dec, true) == DECODE_OK,
            "decodifica pacote maximo");
  VERIFICAR(dec.tamanho_payload == MAX_PAYLOAD && dec.payload[63] == 64,
            "payload maximo preservado");
}

static void testeRejeicoes() {
  std::printf("[PACOTE] rejeicoes de pacotes invalidos\n");
  Pacote pkt = fazerPacoteTelemetria();
  uint8_t buf[TAMANHO_MAXIMO_PACOTE];
  size_t n = codificarPacote(pkt, buf, sizeof(buf));
  Pacote dec;

  // Muito curto.
  VERIFICAR(decodificarPacote(buf, 5, dec, true) == DECODE_MUITO_CURTO,
            "rejeita pacote muito curto");

  // Truncado (falta 1 byte).
  VERIFICAR(decodificarPacote(buf, n - 1, dec, true) == DECODE_TRUNCADO,
            "rejeita pacote truncado");

  // Magic invalido.
  uint8_t mal[TAMANHO_MAXIMO_PACOTE];
  std::memcpy(mal, buf, n);
  mal[0] = 0x00;
  VERIFICAR(decodificarPacote(mal, n, dec, true) == DECODE_MAGIC_INVALIDO,
            "rejeita magic invalido");

  // Versao invalida.
  std::memcpy(mal, buf, n);
  mal[2] = 99;
  VERIFICAR(decodificarPacote(mal, n, dec, true) == DECODE_VERSAO_INVALIDA,
            "rejeita versao invalida");

  // CRC invalido (altera 1 byte do payload sem recalcular CRC).
  std::memcpy(mal, buf, n);
  mal[TAMANHO_CABECALHO] ^= 0xFF;
  VERIFICAR(decodificarPacote(mal, n, dec, true) == DECODE_CRC_INVALIDO,
            "rejeita CRC invalido");

  // Tamanho de payload impossivel (> MAX_PAYLOAD) no cabecalho.
  std::memcpy(mal, buf, n);
  escreverU16LE(&mal[7], MAX_PAYLOAD + 1);
  VERIFICAR(decodificarPacote(mal, n, dec, true) == DECODE_TAMANHO_INVALIDO,
            "rejeita tamanho de payload invalido");

  // Tipo desconhecido em modo estrito.
  std::memcpy(mal, buf, n);
  mal[3] = 0x7F;  // tipo inexistente
  // Recalcula o CRC para isolar a rejeicao por tipo (e nao por CRC).
  {
    uint16_t tam = lerU16LE(&mal[7]);
    size_t offset = TAMANHO_CABECALHO + tam;
    escreverU16LE(&mal[offset], calcularCrc16(mal, offset));
  }
  VERIFICAR(decodificarPacote(mal, n, dec, true) == DECODE_TIPO_DESCONHECIDO,
            "rejeita tipo desconhecido em modo estrito");
  VERIFICAR(decodificarPacote(mal, n, dec, false) == DECODE_OK,
            "aceita tipo desconhecido em modo nao-estrito");
}

static void testeTelemetriaGnss() {
  std::printf("[TELEMETRIA] serializacao GNSS\n");
  Pacote pkt = fazerPacoteTelemetria();
  uint8_t buf[TAMANHO_MAXIMO_PACOTE];
  size_t n = codificarPacote(pkt, buf, sizeof(buf));
  Pacote dec;
  decodificarPacote(buf, n, dec, true);

  TelemetriaGnss t;
  VERIFICAR(lerTelemetria(dec, t), "lerTelemetria retorna true");
  VERIFICAR(t.latitude_1e7 == -211234567, "latitude preservada (negativa)");
  VERIFICAR(t.longitude_1e7 == -451234567, "longitude preservada (negativa)");
  VERIFICAR(t.altitude_mm == 987654, "altitude preservada");
  VERIFICAR(t.tipo_fix == FIX_RTK_FIXED, "tipo de fix preservado");
  VERIFICAR(t.satelites == 24, "satelites preservado");
  VERIFICAR(t.precisao_horizontal_mm == 14, "precisao preservada");
  VERIFICAR(t.ultimo_comando_teclado == '7', "ultimo comando preservado");
}

static void testeEventoTeclado() {
  std::printf("[TECLADO] serializacao de evento\n");
  EventoTeclado e;
  e.uptime_ms = 555000;
  e.tecla = '#';
  e.tipo_evento = EVT_CONFIRMACAO_NUMERO;
  e.valor_numerico = 4294967295u;  // limite superior de uint32
  e.tamanho_entrada = 10;

  Pacote pkt;
  montarPacoteEventoTeclado(pkt, ID_CONTROLADOR, 7, e);
  uint8_t buf[TAMANHO_MAXIMO_PACOTE];
  size_t n = codificarPacote(pkt, buf, sizeof(buf));
  Pacote dec;
  decodificarPacote(buf, n, dec, true);

  EventoTeclado saida;
  VERIFICAR(lerEventoTeclado(dec, saida), "lerEventoTeclado retorna true");
  VERIFICAR(saida.tecla == '#', "tecla preservada");
  VERIFICAR(saida.valor_numerico == 4294967295u, "valor numerico no limite");
  VERIFICAR(saida.tamanho_entrada == 10, "tamanho de entrada preservado");
}

static void testeEnquadramentoUart() {
  std::printf("[UART] enquadramento COBS + decodificacao\n");
  Pacote pkt = fazerPacoteTelemetria();
  uint8_t quadro[TAMANHO_MAXIMO_QUADRO_COBS];
  size_t n = enquadrarParaUart(pkt, quadro, sizeof(quadro));
  VERIFICAR(n > 0, "enquadrarParaUart retorna tamanho > 0");
  VERIFICAR(quadro[n - 1] == 0x00, "quadro termina com delimitador 0x00");

  // Sem contar o delimitador, desenquadra.
  Pacote dec;
  ResultadoDecodificacao r = desenquadrarDeUart(quadro, n - 1, dec, true);
  VERIFICAR(r == DECODE_OK, "desenquadrarDeUart decodifica corretamente");
  VERIFICAR(dec.sequencia == 4242, "sequencia preservada apos UART");
}

static void testeDeteccaoDuplicataESequencia() {
  std::printf("[SEQUENCIA] deteccao de duplicata e faltante\n");
  // Detector simples equivalente ao usado no receptor.
  auto testar = [](uint16_t anterior, bool temAnterior, uint16_t atual,
                   bool& duplicada, bool& faltante) {
    duplicada = false;
    faltante = false;
    if (temAnterior) {
      if (atual == anterior) {
        duplicada = true;
      } else {
        uint16_t esperada = (uint16_t)(anterior + 1);
        if (atual != esperada) faltante = true;
      }
    }
  };
  bool dup, falt;
  testar(10, true, 10, dup, falt);
  VERIFICAR(dup && !falt, "sequencia repetida => duplicada");
  testar(10, true, 11, dup, falt);
  VERIFICAR(!dup && !falt, "sequencia +1 => normal");
  testar(10, true, 13, dup, falt);
  VERIFICAR(!dup && falt, "salto de sequencia => faltante");
  // Rollover 65535 -> 0 nao deve ser tratado como faltante.
  testar(65535, true, 0, dup, falt);
  VERIFICAR(!dup && !falt, "rollover 65535->0 tratado como normal");
}

int main() {
  std::printf("==== Testes do protocolo_comunicacao ====\n\n");
  testeCrcVetoresConhecidos();
  testeCobsRoundTrip();
  testeEncodeDecodeRoundTrip();
  testeMinimoEMaximo();
  testeRejeicoes();
  testeTelemetriaGnss();
  testeEventoTeclado();
  testeEnquadramentoUart();
  testeDeteccaoDuplicataESequencia();

  std::printf("\n==== Resultado: %d/%d testes passaram, %d falha(s) ====\n",
              g_total - g_falhas, g_total, g_falhas);
  return g_falhas == 0 ? 0 : 1;
}
