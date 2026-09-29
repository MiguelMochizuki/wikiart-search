/**
 * app.js
 * Autores: Miguel Mochizuki Silva, Arthur Gomes e Leudo Neto
 * Descrição: Navegação estilo -> artista -> obras da interface web.
 *
 * Os três níveis vêm da engine, cada um da sua estrutura: /api/generos,
 * /api/artistas e /api/busca. Toda resposta traz as métricas da busca, que
 * aparecem no topo de cada tela.
 *
 * A forma da tela segue a estrutura escolhida. A tabela ordenada responde
 * uma lista, e a tela é uma grade. A árvore afunilada responde a vista dos
 * quatro primeiros níveis da árvore, e a tela desenha essa hierarquia:
 * clicar num nó o afunila até a raiz (a engine reorganiza a árvore), e
 * clicar na raiz abre o próximo nível.
 */

'use strict';

/* ==============================
 * Configuração
 * ============================== */

/* Base da API: ?api=... na URL vence, depois o que ficou salvo, senão o padrão
 * do compose. Permite abrir o index.html direto do disco e ainda falar com o
 * container. */
const API = (() => {
	const daUrl = new URLSearchParams(location.search).get('api');
	if (daUrl) {
		try { localStorage.setItem('wikiart:api', daUrl); } catch { /* modo privado */ }
		return daUrl.replace(/\/$/, '');
	}
	let salva = null;
	try { salva = localStorage.getItem('wikiart:api'); } catch { /* idem */ }
	return (salva || 'http://localhost:8080').replace(/\/$/, '');
})();

/* O endpoint de arquivo único do Kaggle é aberto para este dataset (CC0):
 * responde 302 para uma URL assinada do GCS, sem exigir token. */
const IMG_BASE = 'https://www.kaggle.com/api/v1/datasets/download/steubk/wikiart/';

const PAGINA = 30; // obras pedidas à engine por página no modo lista

const NOMES_ED = {
	tabela_ord: 'Tabela ordenada',
	arvore_afunilada: 'Árvore afunilada'
};

/* Um nível da navegação: de onde vêm os dados e onde eles aparecem.
 * O foco é a chave exata do item acessado na árvore. */
const NIVEIS = {
	generos: {
		tela: 'estilos',
		rota: 'generos',
		params: () => ({}),
		foco: (item) => item.nome,
		metricas: 'metricas-estilos',
		grade: 'grade-estilos',
		arvore: 'arvore-estilos',
		filtro: 'filtro-estilos',
		acao: 'Abrir artistas'
	},
	artistas: {
		tela: 'artistas',
		rota: 'artistas',
		params: () => ({ genero: estado.genero }),
		foco: (item) => item.nome,
		metricas: 'metricas-artistas',
		grade: 'grade-artistas',
		arvore: 'arvore-artistas',
		filtro: 'filtro-artistas',
		acao: 'Ver obras'
	},
	obras: {
		tela: 'obras',
		rota: 'busca',
		params: () => ({ genero: estado.genero, artista: estado.artista }),
		foco: (item) => item.id,
		metricas: 'metricas-obras',
		grade: 'grade-obras',
		arvore: 'arvore-obras',
		filtro: null,
		acao: 'Ampliar'
	}
};

/* ==============================
 * Estado
 * ============================== */

const estado = {
	nivel: null,    // chave de NIVEIS da tela aberta
	genero: null,   // nome cru, como está no CSV
	artista: null,  // idem
	lista: [],      // itens do nível no modo lista (a base dos filtros)
	obras: [],      // obras já recebidas (as páginas pedidas até agora)
	totalObras: 0,  // obras do artista na engine, dentro e fora das páginas
	carregando: false,
	renderizadas: 0,
	/* Cresce a cada consulta disparada. Resposta com selo antigo é descartada:
	 * trocar de nível, de estrutura ou afunilar rápido não pode deixar a
	 * resposta atrasada sobrescrever a tela atual. */
	selo: 0
};

const $ = (id) => document.getElementById(id);

/* ==============================
 * Formatação
 * ============================== */

/** Capitaliza cada palavra: "claude monet" -> "Claude Monet" */
function capitalizar(s) {
	return s.replace(/\S+/g, (p) => p.charAt(0).toUpperCase() + p.slice(1));
}

/** Monta o título de exibição a partir do slug do dataset
 *
 * O campo "titulo" vem em slug ("acolman-1-1955") e costuma terminar com o
 * mesmo ano que já mostramos à parte, então o ano repetido é removido.
 */
function tituloBonito(titulo, ano) {
	let t = String(titulo || '').replace(/[-_]+/g, ' ').trim();
	if (ano && t.endsWith(' ' + ano)) t = t.slice(0, -String(ano).length - 1).trim();
	return t ? capitalizar(t) : 'Sem título';
}

function urlImagem(caminho) {
	return IMG_BASE + encodeURIComponent(caminho);
}

const nf = new Intl.NumberFormat('pt-BR');
const nf1 = new Intl.NumberFormat('pt-BR', { minimumFractionDigits: 1, maximumFractionDigits: 1 });
const nf3 = new Intl.NumberFormat('pt-BR', { minimumFractionDigits: 3, maximumFractionDigits: 3 });
const plural = (n, sing, pl) => `${nf.format(n)} ${n === 1 ? sing : pl}`;

/** Tempo de busca legível: as buscas de gênero ficam na casa dos décimos de
 * microssegundo, e "0,000 ms" não diria nada. */
function formatarTempo(ms) {
	if (typeof ms !== 'number') return '—';
	return ms < 1 ? `${nf1.format(ms * 1000)} µs` : `${nf3.format(ms)} ms`;
}

/** Nome e linha de detalhe de um item, conforme o nível */
function descrever(nivel, item) {
	if (nivel === 'generos') {
		return {
			nome: item.nome,
			meta: `${plural(item.obras, 'obra', 'obras')} · ${plural(item.artistas, 'artista', 'artistas')}`
		};
	}
	if (nivel === 'artistas') {
		return { nome: capitalizar(item.nome), meta: plural(item.obras, 'obra', 'obras') };
	}
	return {
		nome: tituloBonito(item.titulo, item.ano),
		meta: item.ano > 0 ? String(item.ano) : 'ano desconhecido'
	};
}

/* ==============================
 * Estrutura escolhida
 * ============================== */

function edAtual() {
	return $('seletor-ed').value;
}

/** Restaura a estrutura da última visita. É conveniência do visitante:
 * sem armazenamento, a interface abre na tabela e segue funcionando. */
function restaurarEd() {
	let salva = null;
	try { salva = localStorage.getItem('wikiart:ed'); } catch { /* modo privado */ }
	if (salva && NOMES_ED[salva]) $('seletor-ed').value = salva;
}

function guardarEd() {
	try { localStorage.setItem('wikiart:ed', edAtual()); } catch { /* idem */ }
}

/* ==============================
 * API
 * ============================== */

/** Consulta uma rota da engine com a estrutura escolhida
 *
 * O URLSearchParams codifica espaço como "+", que o servidor já decodifica.
 * Em erro HTTP, a mensagem da engine (campo "erro") vai junto na exceção.
 */
async function consultar(rota, params) {
	const qs = new URLSearchParams({ ...params, ed: edAtual() });
	const resp = await fetch(`${API}/api/${rota}?${qs}`);
	if (!resp.ok) {
		let detalhe = '';
		try { detalhe = (await resp.json()).erro || ''; } catch { /* corpo sem JSON */ }
		throw new Error(`HTTP ${resp.status}${detalhe ? ' · ' + detalhe : ''}`);
	}
	return resp.json();
}

/* ==============================
 * Navegação (telas + trilha + hash)
 * ============================== */

function mostrarTela(nome) {
	$('tela-estilos').hidden = nome !== 'estilos';
	$('tela-artistas').hidden = nome !== 'artistas';
	$('tela-obras').hidden = nome !== 'obras';
}

function hashGenero(genero) {
	return '#/' + encodeURIComponent(genero);
}

function hashArtista(genero, artista) {
	return hashGenero(genero) + '/' + encodeURIComponent(artista);
}

function montarTrilha() {
	const trilha = $('trilha');
	trilha.textContent = '';

	const nos = [{ rotulo: 'Estilos', hash: '#/' }];
	if (estado.genero) nos.push({ rotulo: estado.genero, hash: hashGenero(estado.genero) });
	if (estado.artista) nos.push({ rotulo: capitalizar(estado.artista), hash: null });

	nos.forEach((no, i) => {
		if (i > 0) {
			const sep = document.createElement('span');
			sep.className = 'trilha-sep';
			sep.textContent = '◆';
			trilha.append(sep);
		}
		const b = document.createElement('button');
		b.type = 'button';
		b.className = 'trilha-item';
		b.textContent = no.rotulo;
		if (i === nos.length - 1) {
			b.setAttribute('aria-current', 'true');
		} else {
			b.addEventListener('click', () => { location.hash = no.hash; });
		}
		trilha.append(b);
	});
}

/** Traduz o hash da URL em estado. Deixa o botão voltar do navegador funcionando. */
function rotear() {
	let partes = [];
	try {
		partes = location.hash.replace(/^#\/?/, '').split('/').filter(Boolean).map(decodeURIComponent);
	} catch { /* hash malformado: volta aos estilos */ }
	const [genero, artista] = partes;

	estado.genero = genero || null;
	estado.artista = (genero && artista) || null;
	montarTrilha();

	if (!estado.genero) abrirNivel('generos');
	else if (!estado.artista) abrirNivel('artistas');
	else abrirNivel('obras');
}

function abrirNivel(nivel) {
	estado.nivel = nivel;
	mostrarTela(NIVEIS[nivel].tela);
	carregar(nivel);
}

/* ==============================
 * Carga de um nível
 * ============================== */

/** Busca um nível na engine e desenha a resposta
 *
 * Parâmetros:
 * string nivel: chave de NIVEIS
 * foco: chave do item a afunilar (só na árvore), ou undefined
 */
async function carregar(nivel, foco) {
	const cfg = NIVEIS[nivel];
	const selo = ++estado.selo;
	estado.carregando = false;

	prepararCabecalho(nivel);
	/* Afunilar mantém a árvore na tela, esmaecida, até a nova forma chegar:
	 * sumir com ela a cada clique faria a página pular. */
	if (foco === undefined) {
		if (nivel === 'obras') $('comparacao-saida').textContent = '';
		$(cfg.metricas).hidden = true;
		$(cfg.grade).textContent = '';
		$(cfg.arvore).textContent = '';
	}
	$(cfg.arvore).classList.add('ocupada');

	const params = cfg.params();
	if (foco !== undefined) params.foco = foco;
	/* Só a primeira página de obras: a engine conta o intervalo pelos tamanhos
	 * guardados na árvore, sem percorrê-lo, e a rolagem pede as seguintes. */
	if (nivel === 'obras') params.limite = PAGINA;

	let dados;
	try {
		dados = await consultar(cfg.rota, params);
	} catch (err) {
		if (selo !== estado.selo) return;
		$(cfg.arvore).classList.remove('ocupada');
		prepararCabecalho(nivel, null);
		avisarEngine(`Falha ao consultar a engine: ${err.message}.`);
		return;
	}
	if (selo !== estado.selo) return;

	esconderAviso();
	renderMetricas($(cfg.metricas), dados);

	const emArvore = Array.isArray(dados.vista);
	$(cfg.grade).hidden = emArvore;
	$(cfg.arvore).hidden = !emArvore;
	if (cfg.filtro) $(cfg.filtro).hidden = emArvore;

	if (emArvore) {
		estado.lista = [];
		estado.obras = [];
		estado.totalObras = 0;
		estado.renderizadas = 0;
		prepararCabecalho(nivel, dados.total);
		renderArvore(nivel, dados);
	} else {
		renderLista(nivel, dados.resultados || [], dados.metricas.total_encontrados);
	}
}

/** Título e resumo da tela
 *
 * Parâmetros:
 * string nivel: chave de NIVEIS
 * total: itens do nível; undefined enquanto busca, null se a busca falhou
 */
function prepararCabecalho(nivel, total) {
	const contagem = (sing, pl) => {
		if (total === undefined) return 'buscando…';
		if (total === null) return '';
		return plural(total, sing, pl);
	};

	if (nivel === 'generos') {
		$('resumo-estilos').textContent = contagem('estilo', 'estilos');
		return;
	}

	if (nivel === 'artistas') {
		$('titulo-artistas').textContent = estado.genero;
		const n = contagem('artista', 'artistas');
		$('resumo-artistas').textContent = typeof total === 'number' ? `${n} neste estilo` : n;
		return;
	}

	$('titulo-obras').textContent = capitalizar(estado.artista);
	const n = contagem('obra encontrada', 'obras encontradas');
	$('resumo-obras').textContent = n ? `em ${estado.genero} · ${n}` : `em ${estado.genero}`;
}

function renderMetricas(caixa, dados) {
	const m = dados.metricas || {};
	const itens = [
		['Estrutura', NOMES_ED[dados.estrutura] || dados.estrutura || '—'],
		['Algoritmo', dados.algoritmo || '—'],
		['Tempo', formatarTempo(m.tempo_ms)],
		['Comparações', typeof m.comparacoes === 'number' ? nf.format(m.comparacoes) : '—']
	];
	/* Rotação só existe na estrutura que se reorganiza ao ser consultada. */
	if (Array.isArray(dados.vista)) {
		itens.push(['Rotações', typeof m.rotacoes === 'number' ? nf.format(m.rotacoes) : '—']);
	}

	caixa.textContent = '';
	for (const [rotulo, valor] of itens) {
		const dl = document.createElement('dl');
		dl.className = 'metrica';
		const dt = document.createElement('dt');
		dt.textContent = rotulo;
		const dd = document.createElement('dd');
		dd.textContent = valor;
		dl.append(dt, dd);
		caixa.append(dl);
	}
	caixa.hidden = false;
}

/* ==============================
 * Comparação entre as estruturas
 * ============================== */

/** Roda a busca de obras atual nas duas estruturas, duas vezes, e explica
 *
 * A segunda rodada mostra o que a árvore aprendeu com a primeira: o que foi
 * aberto há pouco já está no topo. A tabela não muda entre as rodadas.
 */
async function compararEstruturas() {
	const botao = $('botao-comparar');
	const saida = $('comparacao-saida');
	const selo = estado.selo;
	botao.disabled = true;
	saida.textContent = 'comparando…';

	try {
		const params = { genero: estado.genero, artista: estado.artista, limite: PAGINA };
		const primeira = await consultar('comparar', params);
		const segunda = await consultar('comparar', params);
		if (selo !== estado.selo) return;
		renderComparacao(saida, primeira, segunda);
	} catch (err) {
		if (selo === estado.selo) saida.textContent = `Falha ao comparar: ${err.message}.`;
	} finally {
		botao.disabled = false;
	}
}

/** Desenha a tabela da comparação e as frases que a explicam
 *
 * Parâmetros:
 * saida: elemento que recebe o resultado
 * primeira, segunda: respostas de /api/comparar das duas rodadas
 */
function renderComparacao(saida, primeira, segunda) {
	const de = (dados, nome) => dados.comparativo.find((c) => c.estrutura === nome);
	const t1 = de(primeira, 'tabela_ord'), a1 = de(primeira, 'arvore_afunilada');
	const t2 = de(segunda, 'tabela_ord'), a2 = de(segunda, 'arvore_afunilada');

	const linhas = [
		['Agora · comparações', t1.comparacoes, a1.comparacoes],
		['Agora · rotações', '—', a1.rotacoes],
		['Agora · tempo', formatarTempo(t1.tempo_ms), formatarTempo(a1.tempo_ms)],
		['Logo em seguida · comparações', t2.comparacoes, a2.comparacoes],
		['Logo em seguida · rotações', '—', a2.rotacoes],
		['Logo em seguida · tempo', formatarTempo(t2.tempo_ms), formatarTempo(a2.tempo_ms)]
	];

	const tabela = document.createElement('table');
	tabela.className = 'comparacao-tabela';
	const cab = tabela.createTHead().insertRow();
	for (const t of ['', NOMES_ED.tabela_ord, NOMES_ED.arvore_afunilada]) {
		const th = document.createElement('th');
		th.textContent = t;
		cab.append(th);
	}
	const corpo = tabela.createTBody();
	for (const [rotulo, t, a] of linhas) {
		const tr = corpo.insertRow();
		tr.insertCell().textContent = rotulo;
		for (const v of [t, a]) tr.insertCell().textContent = typeof v === 'number' ? nf.format(v) : v;
	}

	const frases = [];
	frases.push(`As duas estruturas devolveram as mesmas ${nf.format(primeira.total_encontrados)} obras` +
		` (a tela pede ${PAGINA} por vez).`);
	frases.push(`Para achar esta página, a tabela fez ${nf.format(t1.comparacoes)} comparações e ` +
		`a árvore, ${nf.format(a1.comparacoes)}` + (a1.rotacoes > 0
			? `, com ${nf.format(a1.rotacoes)} rotações para levar este artista ao topo.`
			: ', sem rotações: um acesso anterior já tinha deixado este artista no topo.'));
	if (a2.rotacoes < a1.rotacoes || a2.comparacoes < a1.comparacoes) {
		frases.push(`Na segunda vez a árvore precisou de ${nf.format(a2.comparacoes)} comparações e ` +
			`${nf.format(a2.rotacoes)} rotações: o que você abriu ficou no topo. ` +
			`A tabela repetiu o mesmo esforço (${nf.format(t2.comparacoes)}).`);
	} else {
		frases.push('Repetir a busca não deixou a árvore mais barata: o artista já estava no topo.');
	}
	const lento = Math.max(t1.tempo_ms, a1.tempo_ms, t2.tempo_ms, a2.tempo_ms);
	frases.push(lento < 1
		? 'Em tempo, todas as respostas levaram menos de 1 ms: ao navegar você não sente essa diferença.'
		: `A resposta mais lenta levou ${formatarTempo(lento)}.`);

	saida.textContent = '';
	const lista = document.createElement('ul');
	lista.className = 'comparacao-frases';
	for (const f of frases) {
		const li = document.createElement('li');
		li.textContent = f;
		lista.append(li);
	}
	saida.append(tabela, lista);
}

/* ==============================
 * Peças comuns
 * ============================== */

/** Moldura com a pintura: entra rebaixada e só aparece no load
 *
 * Parâmetros:
 * obra: obra a mostrar (pode faltar, e aí a moldura fica vazia)
 * string alt: texto alternativo
 */
function criarMoldura(obra, alt) {
	const moldura = document.createElement('div');
	moldura.className = 'moldura';

	const falhar = () => {
		const falha = document.createElement('span');
		falha.className = 'falha';
		falha.textContent = 'imagem indisponível';
		moldura.append(falha);
	};

	if (!obra || !obra.caminho) {
		falhar();
		return moldura;
	}

	const img = document.createElement('img');
	img.loading = 'lazy';
	img.decoding = 'async';
	/* Sem referrer: evita que o Kaggle receba a origem local a cada thumb. */
	img.referrerPolicy = 'no-referrer';
	img.alt = alt;

	/* Os ouvintes vêm antes do src: a thumb começa com opacity 0 e só aparece
	 * no load. Se o src fosse atribuído primeiro, uma imagem já em cache
	 * poderia disparar o load sem ninguém ouvindo e ficar invisível. */
	img.addEventListener('load', () => img.classList.add('pronta'));
	img.addEventListener('error', () => {
		img.remove();
		falhar();
	});
	img.src = urlImagem(obra.caminho);
	if (img.complete && img.naturalWidth > 0) img.classList.add('pronta');

	moldura.append(img);
	return moldura;
}

function criarNumero(n, sing, pl) {
	const span = document.createElement('span');
	const b = document.createElement('b');
	b.textContent = nf.format(n);
	span.append(b, ' ' + (n === 1 ? sing : pl));
	return span;
}

function criarVazio(texto) {
	const p = document.createElement('p');
	p.className = 'vazio';
	p.textContent = texto;
	return p;
}

/** O que clicar na raiz (ou num cartão da lista) faz em cada nível */
function abrirItem(nivel, item) {
	if (nivel === 'generos') {
		location.hash = hashGenero(item.nome);
	} else if (nivel === 'artistas') {
		location.hash = hashArtista(estado.genero, item.nome);
	} else {
		abrirVisor(item, tituloBonito(item.titulo, item.ano));
	}
}

/* ==============================
 * Modo lista (tabela ordenada)
 * ============================== */

function renderLista(nivel, itens, total) {
	if (nivel === 'obras') {
		estado.lista = [];
		estado.obras = itens;
		estado.totalObras = total;
		estado.renderizadas = 0;
		$('grade-obras').textContent = '';
		prepararCabecalho('obras', total);
		renderLoteObras();
		return;
	}

	estado.lista = itens;
	if (nivel === 'generos') {
		const obras = itens.reduce((s, g) => s + g.obras, 0);
		$('resumo-estilos').textContent =
			`${plural(itens.length, 'estilo', 'estilos')} · ${plural(obras, 'obra indexada', 'obras indexadas')}`;
		renderEstilos();
	} else {
		const obras = itens.reduce((s, a) => s + a.obras, 0);
		$('resumo-artistas').textContent =
			`${plural(itens.length, 'artista', 'artistas')} · ${plural(obras, 'obra', 'obras')} neste estilo`;
		renderArtistas();
	}
}

/** Grade de estilos, filtrada pelo campo de busca */
function renderEstilos() {
	if (estado.nivel !== 'generos') return;
	const termo = $('filtro-estilos').value.trim().toLowerCase();
	const lista = estado.lista.filter((g) => g.nome.toLowerCase().includes(termo));

	const grade = $('grade-estilos');
	grade.textContent = '';

	if (!lista.length) {
		grade.append(criarVazio('Nenhum estilo com esse nome.'));
		return;
	}

	let ordem = 0;
	for (const genero of lista) {
		const cartao = document.createElement('button');
		cartao.type = 'button';
		cartao.className = 'cartao-estilo';
		/* --i escalona a animação de entrada; o CSS limita o atraso máximo. */
		cartao.style.setProperty('--i', ordem++);

		const corpo = document.createElement('div');
		corpo.className = 'cartao-corpo';
		const h3 = document.createElement('h3');
		h3.textContent = genero.nome;
		const nums = document.createElement('div');
		nums.className = 'cartao-numeros';
		nums.append(
			criarNumero(genero.obras, 'obra', 'obras'),
			criarNumero(genero.artistas, 'artista', 'artistas')
		);
		corpo.append(h3, nums);

		cartao.append(criarMoldura(genero.capa, `Capa de ${genero.nome}`), corpo);
		cartao.addEventListener('click', () => abrirItem('generos', genero));
		grade.append(cartao);
	}
}

/** Grade de artistas do estilo, filtrada pelo campo de busca */
function renderArtistas() {
	if (estado.nivel !== 'artistas') return;
	const termo = $('filtro-artistas').value.trim().toLowerCase();
	const lista = estado.lista.filter((a) => a.nome.toLowerCase().includes(termo));

	const grade = $('grade-artistas');
	grade.textContent = '';

	if (!lista.length) {
		grade.append(criarVazio(estado.lista.length
			? 'Nenhum artista com esse nome neste estilo.'
			: 'Nenhum artista neste estilo.'));
		return;
	}

	let ordem = 0;
	for (const artista of lista) {
		const cartao = document.createElement('button');
		cartao.type = 'button';
		cartao.className = 'cartao-artista';
		cartao.style.setProperty('--i', ordem++);

		const nomeExibido = capitalizar(artista.nome);
		const nome = document.createElement('span');
		nome.className = 'nome';
		nome.textContent = nomeExibido;

		const pilula = document.createElement('span');
		pilula.className = 'pilula';
		pilula.textContent = nf.format(artista.obras);
		pilula.title = plural(artista.obras, 'obra', 'obras') + ' neste estilo';

		cartao.append(criarMoldura(artista.capa, `Obra de ${nomeExibido}`), nome, pilula);
		cartao.addEventListener('click', () => abrirItem('artistas', artista));
		grade.append(cartao);
	}
}

/** Renderiza as obras recebidas que ainda não estão na grade */
function renderLoteObras() {
	const grade = $('grade-obras');

	if (!estado.obras.length) {
		grade.textContent = '';
		grade.append(criarVazio('A engine não retornou obras para essa combinação.'));
		return;
	}

	const fim = estado.obras.length;
	const frag = document.createDocumentFragment();

	/* O escalonamento reinicia a cada página: a segunda entra rolando, e
	 * contar desde o início da lista deixaria todos com o atraso no teto. */
	for (let i = estado.renderizadas; i < fim; i++) {
		frag.append(criarCartaoObra(estado.obras[i], i - estado.renderizadas));
	}
	grade.append(frag);
	estado.renderizadas = fim;
}

/** Pede à engine a próxima página de obras e a acrescenta à grade
 *
 * Só no modo lista, com a grade à vista. Uma resposta atrasada de outra
 * tela é descartada pelo selo, como em carregar.
 */
async function maisObras() {
	if (estado.nivel !== 'obras' || $('grade-obras').hidden || estado.carregando ||
	    estado.obras.length >= estado.totalObras) return;

	const selo = estado.selo;
	estado.carregando = true;
	try {
		const params = { ...NIVEIS.obras.params(), offset: estado.obras.length, limite: PAGINA };
		const dados = await consultar(NIVEIS.obras.rota, params);
		if (selo !== estado.selo) return;
		estado.obras.push(...(dados.resultados || []));
		renderLoteObras();
	} catch (err) {
		if (selo === estado.selo) avisarEngine(`Falha ao consultar a engine: ${err.message}.`);
	} finally {
		if (selo === estado.selo) {
			estado.carregando = false;
			/* Sentinela ainda à vista (tela alta): re-observar dispara de novo. */
			observadorObras.unobserve($('sentinela'));
			observadorObras.observe($('sentinela'));
		}
	}
}

function criarCartaoObra(obra, ordem) {
	const titulo = tituloBonito(obra.titulo, obra.ano);

	const cartao = document.createElement('article');
	cartao.className = 'cartao-obra';
	cartao.style.setProperty('--i', ordem);

	const info = document.createElement('div');
	info.className = 'obra-info';
	const h3 = document.createElement('h3');
	h3.className = 'obra-titulo';
	h3.textContent = titulo;
	const ano = document.createElement('p');
	ano.className = 'obra-ano';
	ano.textContent = obra.ano > 0 ? obra.ano : 'ano desconhecido';
	info.append(h3, ano);

	cartao.append(criarMoldura(obra, `${titulo}, de ${capitalizar(obra.artista)}`), info);
	cartao.addEventListener('click', () => abrirVisor(obra, titulo));
	return cartao;
}

/* ==============================
 * Modo árvore (árvore afunilada)
 * ============================== */

/** Desenha a vista da árvore: uma fileira por nível, a raiz ocupando a
 * primeira inteira e cada nó dividindo a largura do pai com o irmão
 *
 * A vista vem em layout de heap: a posição p tem os filhos em 2p+1 e 2p+2,
 * e a fileira f vai de 2^f - 1 a 2^(f+1) - 2. Como cada célula mede
 * exatamente metade da do pai, o centro de um filho cai a 25% ou 75% da
 * largura do pai, e é ali que o CSS desce os fios.
 */
function renderArvore(nivel, dados) {
	const caixa = $(NIVEIS[nivel].arvore);
	const vista = dados.vista;
	const niveis = dados.niveis || Math.log2(vista.length + 1);

	caixa.classList.remove('ocupada');
	caixa.textContent = '';

	if (!vista[0]) {
		const vazio = {
			generos: 'A árvore de estilos está vazia.',
			artistas: 'Nenhum artista neste estilo.',
			obras: 'A engine não retornou obras para essa combinação.'
		};
		caixa.append(criarVazio(vazio[nivel]));
		return;
	}

	const visiveis = vista.filter(Boolean).length;
	const legenda = document.createElement('p');
	legenda.className = 'arvore-legenda';
	const contagem = document.createElement('b');
	contagem.textContent = `${nf.format(visiveis)} de ${nf.format(dados.total)}`;
	legenda.append(
		contagem,
		` à vista. A raiz é o último acesso: clique num nó para afunilá-lo até o topo, `
		+ `e na raiz para ${NIVEIS[nivel].acao.toLowerCase()}.`
	);
	caixa.append(legenda);

	for (let f = 0; f < niveis; f++) {
		const fileira = document.createElement('div');
		fileira.className = 'arvore-fileira';
		fileira.dataset.fileira = f;
		fileira.style.setProperty('--colunas', 2 ** f);

		for (let p = 2 ** f - 1; p < 2 ** (f + 1) - 1; p++) {
			const celula = document.createElement('div');
			celula.className = 'arvore-celula';

			const no = vista[p];
			if (no) {
				if (f < niveis - 1) {
					celula.classList.toggle('filho-esq', Boolean(vista[2 * p + 1]));
					celula.classList.toggle('filho-dir', Boolean(vista[2 * p + 2]));
				}
				celula.append(criarNo(nivel, no, p, f === niveis - 1));
			}
			fileira.append(celula);
		}
		caixa.append(fileira);
	}
}

/** Um nó da vista
 *
 * Parâmetros:
 * string nivel: chave de NIVEIS
 * no: posição da vista ({ item, descendentes })
 * number pos: posição no heap (0 é a raiz)
 * boolean ultimaFileira: se está na fileira de baixo, onde os descendentes
 *                        ficam todos fora da vista
 */
function criarNo(nivel, no, pos, ultimaFileira) {
	const { item, descendentes } = no;
	const raiz = pos === 0;
	const { nome, meta } = descrever(nivel, item);
	const capa = nivel === 'obras' ? item : item.capa;

	const b = document.createElement('button');
	b.type = 'button';
	b.className = raiz ? 'no no-raiz' : 'no';
	b.style.setProperty('--i', pos);

	const texto = document.createElement('div');
	texto.className = 'no-texto';

	if (raiz) {
		const marca = document.createElement('span');
		marca.className = 'no-marca';
		marca.textContent = 'Raiz';
		texto.append(marca);
	}

	const nomeEl = document.createElement(raiz ? 'h3' : 'span');
	nomeEl.className = 'no-nome';
	nomeEl.textContent = nome;
	const metaEl = document.createElement('span');
	metaEl.className = 'no-meta';
	metaEl.textContent = meta;
	texto.append(nomeEl, metaEl);

	/* Nos dois primeiros níveis a pintura é de capa: a raiz diz de quem é. */
	if (raiz && nivel !== 'obras' && capa) {
		const credito = document.createElement('span');
		credito.className = 'no-credito';
		credito.textContent =
			`Capa: ${tituloBonito(capa.titulo, capa.ano)}, de ${capitalizar(capa.artista)}`
			+ (capa.ano > 0 ? ` (${capa.ano})` : '');
		texto.append(credito);
	}

	if (raiz) {
		const acao = document.createElement('span');
		acao.className = 'no-acao';
		acao.textContent = `${NIVEIS[nivel].acao} →`;
		texto.append(acao);
	}

	b.append(criarMoldura(capa, nome), texto);

	/* Na última fileira nada abaixo aparece: o selo diz quanto ficou de fora. */
	if (ultimaFileira && descendentes > 0) {
		const ocultos = document.createElement('span');
		ocultos.className = 'no-ocultos';
		ocultos.textContent = `+${nf.format(descendentes)}`;
		ocultos.title = `${plural(descendentes, 'item', 'itens')} abaixo deste nó`;
		b.append(ocultos);
	}

	if (raiz) {
		b.title = `${NIVEIS[nivel].acao}: ${nome}`;
		b.setAttribute('aria-label', `${NIVEIS[nivel].acao}: ${nome}`);
		b.addEventListener('click', () => abrirItem(nivel, item));
	} else {
		b.title = 'Levar ao topo da árvore';
		b.setAttribute('aria-label', `Levar ${nome} ao topo da árvore`);
		b.addEventListener('click', () => carregar(nivel, NIVEIS[nivel].foco(item)));
	}
	return b;
}

/* ==============================
 * Visor
 * ============================== */

function abrirVisor(obra, titulo) {
	const img = $('visor-img');
	img.referrerPolicy = 'no-referrer';
	img.src = urlImagem(obra.caminho);
	img.alt = titulo;
	$('visor-legenda').textContent =
		`${titulo} · ${capitalizar(obra.artista)}${obra.ano > 0 ? ' · ' + obra.ano : ''} · ${obra.genero}`;
	$('visor').hidden = false;
}

function fecharVisor() {
	$('visor').hidden = true;
	$('visor-img').src = '';
}

/* ==============================
 * Avisos e status
 * ============================== */

/** Mostra um aviso com a receita de conserto correspondente
 *
 * Parâmetros:
 * string mensagem: o que falhou
 * string comando: comando que resolve
 * string rodape: texto entre a mensagem e o comando
 */
function mostrarAviso(mensagem, comando, rodape) {
	const aviso = $('aviso');
	aviso.textContent = '';
	const b = document.createElement('b');
	b.textContent = mensagem + ' ';
	const code = document.createElement('code');
	code.textContent = comando;
	aviso.append(b, rodape + ' ', code);
	aviso.hidden = false;
}

function avisarEngine(mensagem) {
	mostrarAviso(mensagem, 'docker compose up -d --build engine',
		`A engine responde em ${API}. Confira se ela está no ar e atualizada:`);
}

function esconderAviso() {
	$('aviso').hidden = true;
}

async function checarStatus() {
	const ponto = document.querySelector('.ponto');
	const texto = $('status-texto');
	try {
		const resp = await fetch(`${API}/api/status`);
		const dados = await resp.json();
		ponto.dataset.estado = 'online';
		texto.textContent = `engine online · ${nf.format(dados.total_obras)} obras`;
	} catch {
		ponto.dataset.estado = 'offline';
		texto.textContent = 'engine offline';
		avisarEngine('Não consegui falar com a engine.');
	}
}

/* ==============================
 * Atmosfera
 * ============================== */

const PETALAS = 16;

/** Povoa o fundo com pétalas à deriva
 *
 * Cada uma sobe uma vez a cada ciclo; o que varia entre elas é a coluna, o
 * tamanho, a duração e o desvio lateral. O atraso negativo faz a animação
 * começar no meio: a tela já abre com pétalas espalhadas, em vez de todas
 * saindo juntas da borda de baixo.
 */
function semearPetalas() {
	if (window.matchMedia('(prefers-reduced-motion: reduce)').matches) return;

	const campo = $('particulas');
	const frag = document.createDocumentFragment();

	for (let i = 0; i < PETALAS; i++) {
		const dur = 22 + Math.random() * 26;
		const p = document.createElement('i');
		p.className = 'particula';
		p.style.setProperty('--x', (Math.random() * 100).toFixed(2) + '%');
		p.style.setProperty('--t', (4 + Math.random() * 6).toFixed(1) + 'px');
		p.style.setProperty('--dur', dur.toFixed(1) + 's');
		p.style.setProperty('--atraso', (-Math.random() * dur).toFixed(1) + 's');
		p.style.setProperty('--desvio', Math.round(-90 + Math.random() * 180) + 'px');
		frag.append(p);
	}
	campo.append(frag);
}

/* ==============================
 * Início
 * ============================== */

/* Pede a próxima página quando a sentinela do fim da grade chega perto. */
const observadorObras = new IntersectionObserver((entradas) => {
	if (entradas[0].isIntersecting) maisObras();
}, { rootMargin: '600px' });

function iniciar() {
	semearPetalas();
	restaurarEd();

	/* Explicação das métricas sob cada painel */
	const modelo = $('modelo-ajuda');
	document.querySelectorAll('.metricas').forEach((caixa) => {
		caixa.after(modelo.content.cloneNode(true));
	});
	$('botao-comparar').addEventListener('click', compararEstruturas);

	$('filtro-estilos').addEventListener('input', renderEstilos);
	$('filtro-artistas').addEventListener('input', renderArtistas);
	$('seletor-ed').addEventListener('change', () => {
		guardarEd();
		if (estado.nivel) carregar(estado.nivel);
	});
	$('visor-fechar').addEventListener('click', fecharVisor);
	$('visor').addEventListener('click', (e) => { if (e.target === $('visor')) fecharVisor(); });
	document.addEventListener('keydown', (e) => { if (e.key === 'Escape') fecharVisor(); });
	window.addEventListener('hashchange', rotear);

	/* Rolagem infinita: a sentinela fica no fim da grade de obras. */
	observadorObras.observe($('sentinela'));

	checarStatus();
	rotear();
}

iniciar();
