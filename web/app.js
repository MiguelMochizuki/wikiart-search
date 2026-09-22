/**
 * app.js
 * Autores: Miguel Mochizuki Silva, Arthur Gomes e Leudo Neto
 * Descrição: Navegação estilo -> artista -> obras da interface web.
 *
 * As duas primeiras listas saem de indice.json (estático, ~86 KB) porque a
 * engine só responde obras: montá-las pela API custaria baixar o gênero
 * inteiro só para extrair nomes. A busca final — o passo que o trabalho quer
 * medir — vai em /api/busca e traz junto as métricas da estrutura escolhida.
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

const PAGINA = 60; // obras renderizadas por lote

/* ==============================
 * Estado
 * ============================== */

const estado = {
	indice: null,
	estilo: null,   // objeto do índice
	artista: null,  // nome cru, como está no CSV
	obras: [],
	renderizadas: 0,
	/* Cresce a cada busca disparada. Resposta com selo antigo é descartada:
	 * trocar de artista ou de estrutura rápido não pode deixar a resposta
	 * atrasada sobrescrever a tela atual. */
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
const plural = (n, sing, pl) => `${nf.format(n)} ${n === 1 ? sing : pl}`;

/* ==============================
 * Navegação (telas + trilha + hash)
 * ============================== */

function mostrarTela(nome) {
	$('tela-estilos').hidden = nome !== 'estilos';
	$('tela-artistas').hidden = nome !== 'artistas';
	$('tela-obras').hidden = nome !== 'obras';
}

function montarTrilha() {
	const trilha = $('trilha');
	trilha.textContent = '';

	const nos = [{ rotulo: 'Estilos', hash: '#/' }];
	if (estado.estilo) {
		nos.push({ rotulo: estado.estilo.nome, hash: '#/' + encodeURIComponent(estado.estilo.nome) });
	}
	if (estado.artista) {
		nos.push({ rotulo: capitalizar(estado.artista), hash: null });
	}

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
	if (!estado.indice) return;

	const partes = location.hash.replace(/^#\/?/, '').split('/').filter(Boolean).map(decodeURIComponent);
	const [nomeEstilo, nomeArtista] = partes;

	const estilo = nomeEstilo
		? estado.indice.estilos.find((e) => e.nome === nomeEstilo)
		: null;

	if (!estilo) {
		estado.estilo = null;
		estado.artista = null;
		montarTrilha();
		mostrarTela('estilos');
		renderEstilos();
		return;
	}

	estado.estilo = estilo;

	if (!nomeArtista || !estilo.artistas.some((a) => a.nome === nomeArtista)) {
		estado.artista = null;
		montarTrilha();
		mostrarTela('artistas');
		renderArtistas();
		return;
	}

	estado.artista = nomeArtista;
	montarTrilha();
	mostrarTela('obras');
	buscarObras();
}

/* ==============================
 * Nível 1: estilos
 * ============================== */

function renderEstilos() {
	const termo = $('filtro-estilos').value.trim().toLowerCase();
	const lista = estado.indice.estilos.filter((e) => e.nome.toLowerCase().includes(termo));

	const grade = $('grade-estilos');
	grade.textContent = '';

	if (!lista.length) {
		grade.append(criarVazio('Nenhum estilo com esse nome.'));
		return;
	}

	let ordem = 0;
	for (const estilo of lista) {
		const cartao = document.createElement('button');
		cartao.type = 'button';
		cartao.className = 'cartao-estilo';
		/* --i escalona a animação de entrada; o CSS limita o atraso máximo. */
		cartao.style.setProperty('--i', ordem++);

		const h3 = document.createElement('h3');
		h3.textContent = estilo.nome;

		const nums = document.createElement('div');
		nums.className = 'cartao-numeros';
		nums.append(
			criarNumero(estilo.obras, 'obra', 'obras'),
			criarNumero(estilo.artistas.length, 'artista', 'artistas')
		);

		cartao.append(h3, nums);
		cartao.addEventListener('click', () => {
			location.hash = '#/' + encodeURIComponent(estilo.nome);
		});
		grade.append(cartao);
	}
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

/* ==============================
 * Nível 2: artistas do estilo
 * ============================== */

function renderArtistas() {
	const estilo = estado.estilo;
	$('titulo-artistas').textContent = estilo.nome;
	$('resumo-artistas').textContent =
		`${plural(estilo.artistas.length, 'artista', 'artistas')} · ${plural(estilo.obras, 'obra', 'obras')} neste estilo`;

	const termo = $('filtro-artistas').value.trim().toLowerCase();
	const lista = estilo.artistas.filter((a) => a.nome.toLowerCase().includes(termo));

	const grade = $('grade-artistas');
	grade.textContent = '';

	if (!lista.length) {
		grade.append(criarVazio('Nenhum artista com esse nome neste estilo.'));
		return;
	}

	let ordem = 0;
	for (const artista of lista) {
		const cartao = document.createElement('button');
		cartao.type = 'button';
		cartao.className = 'cartao-artista';
		cartao.style.setProperty('--i', ordem++);

		const nome = document.createElement('span');
		nome.className = 'nome';
		nome.textContent = capitalizar(artista.nome);

		const pilula = document.createElement('span');
		pilula.className = 'pilula';
		pilula.textContent = nf.format(artista.obras);
		pilula.title = plural(artista.obras, 'obra', 'obras') + ' neste estilo';

		cartao.append(nome, pilula);
		cartao.addEventListener('click', () => {
			location.hash = '#/' + encodeURIComponent(estilo.nome) + '/' + encodeURIComponent(artista.nome);
		});
		grade.append(cartao);
	}
}

/* ==============================
 * Nível 3: obras do artista no estilo
 * ============================== */

async function buscarObras() {
	const { estilo, artista } = estado;

	$('titulo-obras').textContent = capitalizar(artista);
	$('resumo-obras').textContent = `em ${estilo.nome} · buscando…`;
	$('metricas').hidden = true;
	$('grade-obras').textContent = '';
	estado.obras = [];
	estado.renderizadas = 0;

	const selo = ++estado.selo;
	const ed = $('seletor-ed').value;
	const url = `${API}/api/busca?genero=${encodeURIComponent(estilo.nome)}`
		+ `&artista=${encodeURIComponent(artista)}&ed=${encodeURIComponent(ed)}`;

	let dados;
	try {
		const resp = await fetch(url);
		if (!resp.ok) throw new Error(`HTTP ${resp.status}`);
		dados = await resp.json();
	} catch (err) {
		if (selo !== estado.selo) return;
		$('resumo-obras').textContent = `em ${estilo.nome}`;
		avisarEngine(`Falha ao consultar a engine: ${err.message}.`);
		return;
	}

	if (selo !== estado.selo) return;

	esconderAviso();
	estado.obras = dados.resultados || [];
	$('resumo-obras').textContent =
		`em ${estilo.nome} · ${plural(estado.obras.length, 'obra encontrada', 'obras encontradas')}`;

	renderMetricas(dados);
	renderLoteObras();
}

function renderMetricas(dados) {
	const m = dados.metricas || {};
	const itens = [
		['Estrutura', dados.estrutura || '—'],
		['Algoritmo', dados.algoritmo || '—'],
		['Tempo', typeof m.tempo_ms === 'number' ? `${m.tempo_ms.toFixed(3)} ms` : '—'],
		['Comparações', typeof m.comparacoes === 'number' ? nf.format(m.comparacoes) : '—']
	];

	const caixa = $('metricas');
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

/** Renderiza o próximo lote de obras (a rolagem pede os seguintes) */
function renderLoteObras() {
	const grade = $('grade-obras');

	if (!estado.obras.length) {
		grade.textContent = '';
		grade.append(criarVazio('A engine não retornou obras para essa combinação.'));
		return;
	}

	const fim = Math.min(estado.renderizadas + PAGINA, estado.obras.length);
	const frag = document.createDocumentFragment();

	/* O escalonamento reinicia a cada lote: o segundo lote entra rolando, e
	 * contar desde o início da lista deixaria todos com o atraso no teto. */
	for (let i = estado.renderizadas; i < fim; i++) {
		frag.append(criarCartaoObra(estado.obras[i], i - estado.renderizadas));
	}
	grade.append(frag);
	estado.renderizadas = fim;
}

function criarCartaoObra(obra, ordem) {
	const titulo = tituloBonito(obra.titulo, obra.ano);

	const cartao = document.createElement('article');
	cartao.className = 'cartao-obra';
	cartao.style.setProperty('--i', ordem);

	const moldura = document.createElement('div');
	moldura.className = 'moldura';

	const img = document.createElement('img');
	img.loading = 'lazy';
	img.decoding = 'async';
	/* Sem referrer: evita que o Kaggle receba a origem local a cada thumb. */
	img.referrerPolicy = 'no-referrer';
	img.alt = `${titulo}, de ${capitalizar(obra.artista)}`;

	/* Os ouvintes vêm antes do src: a thumb começa com opacity 0 e só aparece
	 * no load. Se o src fosse atribuído primeiro, uma imagem já em cache
	 * poderia disparar o load sem ninguém ouvindo e ficar invisível. */
	img.addEventListener('load', () => img.classList.add('pronta'));
	img.addEventListener('error', () => {
		img.remove();
		const falha = document.createElement('span');
		falha.className = 'falha';
		falha.textContent = 'imagem indisponível';
		moldura.append(falha);
	});
	img.src = urlImagem(obra.caminho);
	if (img.complete && img.naturalWidth > 0) img.classList.add('pronta');

	moldura.append(img);

	const info = document.createElement('div');
	info.className = 'obra-info';
	const h3 = document.createElement('h3');
	h3.className = 'obra-titulo';
	h3.textContent = titulo;
	const ano = document.createElement('p');
	ano.className = 'obra-ano';
	ano.textContent = obra.ano > 0 ? obra.ano : 'ano desconhecido';
	info.append(h3, ano);

	cartao.append(moldura, info);
	cartao.addEventListener('click', () => abrirVisor(obra, titulo));
	return cartao;
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
	mostrarAviso(mensagem, 'docker compose up -d engine',
		`A engine responde em ${API}. Confira se ela está no ar:`);
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

async function iniciar() {
	semearPetalas();

	$('filtro-estilos').addEventListener('input', renderEstilos);
	$('filtro-artistas').addEventListener('input', renderArtistas);
	$('seletor-ed').addEventListener('change', () => { if (estado.artista) buscarObras(); });
	$('visor-fechar').addEventListener('click', fecharVisor);
	$('visor').addEventListener('click', (e) => { if (e.target === $('visor')) fecharVisor(); });
	document.addEventListener('keydown', (e) => { if (e.key === 'Escape') fecharVisor(); });
	window.addEventListener('hashchange', rotear);

	/* Rolagem infinita: a sentinela fica no fim da grade de obras. */
	new IntersectionObserver((entradas) => {
		if (entradas[0].isIntersecting && estado.renderizadas < estado.obras.length) {
			renderLoteObras();
		}
	}, { rootMargin: '600px' }).observe($('sentinela'));

	checarStatus();

	try {
		const resp = await fetch('indice.json');
		if (!resp.ok) throw new Error(`HTTP ${resp.status}`);
		estado.indice = await resp.json();
	} catch (err) {
		$('resumo-estilos').textContent = '';
		mostrarAviso(`Não consegui carregar indice.json (${err.message}).`,
			'uv run python scripts/gerar_indice.py',
			'O índice é gerado a partir do metadados.csv; gere-o com:');
		return;
	}

	$('resumo-estilos').textContent =
		`${plural(estado.indice.estilos.length, 'estilo', 'estilos')} · `
		+ `${plural(estado.indice.total_obras, 'obra indexada', 'obras indexadas')}`;

	rotear();
}

iniciar();
