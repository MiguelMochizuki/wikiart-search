/**
 * resultados.js
 * Autores: Miguel Mochizuki Silva, Arthur Gomes e Leudo Neto
 * Descrição: Tela "Resultados". Lê o CSV do benchmark (results/bench_80k.csv)
 *            e conta, em quatro cenários, o que cada estrutura melhora.
 *
 * Os números vêm do CSV; só as frases de causa são fixas. Os gráficos são
 * barras em HTML e CSS, que herdam as cores do tema sem biblioteca.
 */

const CSV_BENCH = 'results/bench_80k.csv';
const NOME_ED = { tabela_ord: 'Tabela ordenada', arvore_afunilada: 'Árvore afunilada' };
const fmt = (n, d = 2) => n.toLocaleString('pt-BR', { minimumFractionDigits: d, maximumFractionDigits: d });

/** Lê o CSV e indexa as linhas por "estrutura/operacao" */
async function lerBench() {
	const resp = await fetch(CSV_BENCH);
	if (!resp.ok) throw new Error(`HTTP ${resp.status} em ${CSV_BENCH}`);
	const [cab, ...linhas] = (await resp.text()).trim().split('\n').map((l) => l.split(','));
	const dados = {};
	for (const l of linhas) {
		const r = Object.fromEntries(cab.map((c, i) => [c, isNaN(l[i]) ? l[i] : Number(l[i])]));
		dados[`${r.estrutura}/${r.operacao}`] = r;
	}
	return dados;
}

function el(tag, classe, texto) {
	const e = document.createElement(tag);
	if (classe) e.className = classe;
	if (texto !== undefined) e.textContent = texto;
	return e;
}

/** Grupo de barras: uma por linha {rotulo, valor, destaque}, na escala do maior valor */
function barras(titulo, unidade, linhas) {
	const max = Math.max(...linhas.map((l) => l.valor));
	const bloco = el('div', 'res-barras');
	bloco.append(el('h4', null, titulo));
	for (const l of linhas) {
		const linha = el('div', 'res-linha');
		const barra = el('span', 'res-barra');
		barra.style.width = `${Math.max((l.valor / max) * 100, 1)}%`;
		barra.dataset.estrutura = l.estrutura;
		linha.append(el('span', 'res-rotulo', l.rotulo), barra, el('span', 'res-valor', `${fmt(l.valor)} ${unidade}`));
		linha.title = `${l.rotulo}: ${fmt(l.valor, 4)} ${unidade}`;
		bloco.append(linha);
	}
	return bloco;
}

/** Laboratório: repete a carga na engine, ao vivo, com /api/insercao
 *
 * A engine monta índices descartáveis, sem tocar nos que atendem a navegação,
 * e o servidor fica ocupado enquanto mede (cerca de meio segundo no acervo completo).
 */
function laboratorioCarga() {
	const lab = el('div', 'res-lab');
	const botao = el('button', 'botao-comparar', 'Medir agora na engine');
	const saida = el('div', 'comparacao-saida');
	saida.setAttribute('aria-live', 'polite');
	botao.type = 'button';
	botao.addEventListener('click', async () => {
		botao.disabled = true;
		saida.textContent = 'medindo…';
		try {
			const resp = await fetch(`${API}/api/insercao`);
			if (!resp.ok) throw new Error(`HTTP ${resp.status}`);
			const d = await resp.json();
			const linhas = d.cargas.map((c) => ({
				rotulo: `${NOME_ED[c.estrutura].split(' ')[0]}, ${c.ordem === 'ordenada' ? 'em ordem' : 'embaralhada'}`,
				valor: c.ms, estrutura: c.estrutura }));
			saida.replaceChildren(barras(`Medido agora: carga de ${d.itens.toLocaleString('pt-BR')} itens`, 'ms', linhas));
		} catch (e) {
			saida.textContent = `Falha ao medir: ${e.message}. A engine está no ar em ${API}?`;
		} finally {
			botao.disabled = false;
		}
	});
	lab.append(el('p', 'comparacao-texto', 'Os números acima vêm de um benchmark gravado. Aqui a engine refaz a carga agora, na sua máquina, com as duas ordens de chegada.'), botao, saida);
	return lab;
}

/** Cartão de cenário: número, título, vencedor, causa e gráficos */
function cenario(num, titulo, vencedor, causa, graficos) {
	const sec = el('article', 'res-cenario');
	const cab = el('header');
	cab.append(el('span', 'ato-num', num), el('h3', null, titulo), el('span', 'res-vence', vencedor));
	sec.append(cab, el('p', 'res-causa', causa), ...graficos);
	return sec;
}

const us = (r) => r.media_ms * 1000;
const T = 'tabela_ord', A = 'arvore_afunilada';

function montarResultados(d) {
	const caixa = el('div', 'res-lista');

	/* 1. Leitura aleatória */
	const ops = [['buscar_genero', 'Um estilo'], ['buscar_artistas_genero', 'Artistas de um estilo'],
		['buscar_obras_artista', 'Obras de um artista'], ['buscar_obras_pagina', 'Página de 30 obras']];
	const razao = us(d[`${A}/buscar_obras_artista`]) / us(d[`${T}/buscar_obras_artista`]);
	caixa.append(cenario('I', 'Leitura aleatória', `Tabela, ${fmt(razao, 1)}x nas obras do artista`,
		'As comparações quase empatam, mas a tabela percorre um array contíguo e a árvore salta entre 80 mil nós no heap, ainda reorganizando o caminho a cada busca.',
		ops.map(([op, nome]) => barras(nome, 'µs', [T, A].map((e) =>
			({ rotulo: NOME_ED[e], valor: us(d[`${e}/${op}`]), estrutura: e }))))));

	/* 2. Localidade */
	const ra = d[`${A}/buscar_obras_artista`], rl = d[`${A}/buscar_obras_artista_local`];
	caixa.append(cenario('II', 'Leitura com localidade', 'Tabela, mas só a árvore se adapta',
		`Com poucas obras em foco, a árvore as mantém perto da raiz: as rotações por busca caem de ${fmt(ra.media_rotacoes)} para ${fmt(rl.media_rotacoes)}. A tabela também melhora (cache do processador) e segue na frente.`,
		[['buscar_obras_artista', 'Obras de um artista'], ['buscar_obras_pagina', 'Página de 30 obras']].map(([op, nome]) =>
			barras(nome, 'µs', [T, A].flatMap((e) => [
				{ rotulo: `${NOME_ED[e]}, aleatório`, valor: us(d[`${e}/${op}`]), estrutura: e },
				{ rotulo: `${NOME_ED[e]}, com foco`, valor: us(d[`${e}/${op}_local`]), estrutura: e }])))));

	/* 3. Carga */
	const carga = (e) => d[`${e}/buscar_genero`].insercao_ms;
	caixa.append(cenario('III', 'Carga dos três níveis', `Árvore, ${fmt(carga(T) / carga(A), 1)}x com chegada embaralhada`,
		'Inserir no meio do array desloca a cauda (O(n)); a árvore só desce e religa ponteiros (O(log n) amortizado). Com as chaves já em ordem, inserir no fim não desloca nada, e as duas carregam em milissegundos.',
		[barras('Tempo de carga de 82.151 itens', 'ms', [
			{ rotulo: 'Tabela, embaralhada', valor: carga(T), estrutura: T },
			{ rotulo: 'Árvore, embaralhada', valor: carga(A), estrutura: A },
			{ rotulo: 'Tabela, em ordem', valor: carga(`${T}_carga_ordenada`), estrutura: T },
			{ rotulo: 'Árvore, em ordem', valor: carga(`${A}_carga_ordenada`), estrutura: A }]), laboratorioCarga()]));

	/* 4. Veredito */
	const v = el('article', 'res-cenario');
	const cab = el('header');
	cab.append(el('span', 'ato-num', 'IV'), el('h3', null, 'Quando usar cada uma'));
	const lista = el('ul', 'res-veredito');
	[['Tabela ordenada', 'acervo estável e lido muitas vezes, como este: a leitura é a mais rápida e a carga só acontece uma vez.'],
		['Árvore afunilada', 'dados que chegam sem ordem e crescem, ou acesso concentrado em poucos itens: insere sem deslocar e se adapta ao uso.']]
		.forEach(([n, t]) => { const li = el('li'); li.append(el('strong', null, n + ': '), t); lista.append(li); });
	v.append(cab, lista, el('p', 'res-causa', 'Análise completa e comparação com o algoritmo clássico em results/RESULTADOS.md.'));
	caixa.append(v);
	return caixa;
}

let resultadosCarregados = false;

/** Carrega o CSV uma vez e desenha a tela */
async function carregarResultados() {
	if (resultadosCarregados) return;
	const alvo = $('resultados-conteudo');
	alvo.textContent = 'Carregando o benchmark…';
	try {
		alvo.replaceChildren(montarResultados(await lerBench()));
		resultadosCarregados = true;
	} catch (e) {
		alvo.textContent = `Não foi possível ler ${CSV_BENCH} (${e.message}). Sirva a interface pelo docker compose para a pasta results/ ficar disponível.`;
	}
}
