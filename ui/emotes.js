const groups = [
 ['Cumprimentos','party',[
 ['IdleWave','Acenar','Um cumprimento à distância.'],['IdleCivilWarCheer','Grito de guerra'],['IdleSalute','Saudar'],['IdleSilentBow','Reverência'],['IdleGetAttention','Chamar atenção'],['IdleLookFar','Olhar ao longe'],['IdleMT_DoorBang','Bater à porta']]],
 ['Reações','health',[
 ['IdleApplaud2','Bater palmas'],['IdleApplaud4','Aplaudir'],['IdleApplaud5','Aplauso alto'],['IdleLaugh','Rir'],['IdleSurrender','Render-se'],['IdleCowerEnter','Medo'],['IdleWipeBrow','Enxugar o suor'],['IdleWounded_02','Ferido']]],
 ['Posturas','person',[
 ['IdleLayDown','Deitar'],['IdleWarmHandsStanding','Aquecer mãos'],['IdleWarmHandsCrouched','Aquecer sentado'],['IdleGrave_01','Orar'],['IdlePray','Venerar'],['IdleSitCrossLeggedEnter','Sentar no chão'],['IdleKneelingEnter','Ajoelhar'],['IdleWounded_03','Relaxar sentado']]],
 ['Diálogo','class',[
 ['OffsetArmsCrossedStart','Cruzar braços'],['IdleGrave_02','Postura formal'],['IdleHandsBehindBack','Mãos para trás'],['IdleExamine','Examinar'],['IdleStudy','Estudar'],['IdleDialogueHandOnChinGesture','Refletir'],['IdlePointFar_01','Apontar']]],
 ['Atividades','professions',[
 ['IdleDrink','Beber'],['IdleEatingStandingStart','Comer'],['IdleLooseSweepingStart','Varrer'],['IdleHoe','Usar enxada'],['IdleRitualStart','Ritual'],['IdleNoteRead','Ler bilhete'],['IdleBook_PageTurn','Ler livro']]],
 ['Diversão','spells',[
 ['IdleCiceroDance1','Dança I'],['IdleCiceroDance2','Dança II'],['IdleCiceroDance3','Dança III'],['IdleDrumStart','Tocar tambor'],['IdleFluteStart','Tocar flauta'],['IdleLuteStart','Tocar alaúde'],['IdleBlowHornImperial','Trompa imperial'],['IdleBlowHornStormcloak','Trompa nórdica']]]
];

let groupIndex=0,selected=0,toastTimer;
const shell=document.getElementById('shell'),nodeContainer=document.getElementById('nodes');
function preview(index){const group=groups[groupIndex],item=group[2][index];document.getElementById('preview-name').textContent=item[1];document.getElementById('center-sub').textContent=group[0].toUpperCase();document.getElementById('center-art').innerHTML=sketch(artByGroup[groupIndex][index],index);}
function choose(index){window.EmotesSP.send({action:'play',id:groups[groupIndex][2][index][0]});}
function render(){document.querySelectorAll('#categories button').forEach((b,i)=>{b.classList.toggle('active',i===groupIndex);b.setAttribute('aria-pressed',String(i===groupIndex));});nodeContainer.replaceChildren();groups[groupIndex][2].forEach((item,i)=>{const angle=-Math.PI/2+(i/groups[groupIndex][2].length)*Math.PI*2,b=document.createElement('button');b.className='emote-node'+(i===selected?' is-selected':'');b.style.left=(50+40*Math.cos(angle))+'%';b.style.top=(50+40*Math.sin(angle))+'%';b.setAttribute('aria-label',item[1]);b.setAttribute('aria-pressed',String(i===selected));b.innerHTML=sketch(artByGroup[groupIndex][i],i)+'<span class="node-label"></span>';b.querySelector('.node-label').textContent=item[1];b.addEventListener('mouseenter',()=>preview(i));b.addEventListener('focus',()=>preview(i));b.addEventListener('click',()=>choose(i));nodeContainer.append(b);});preview(selected);}
groups.forEach((g,i)=>{const angle=-Math.PI/2+i*Math.PI/3,b=document.createElement('button');b.className='category-node';b.style.left=(50+24*Math.cos(angle))+'%';b.style.top=(50+24*Math.sin(angle))+'%';b.innerHTML=sketch(categoryArt[i],i)+'<span class="category-label"></span>';b.querySelector('span').textContent=g[0];b.setAttribute('aria-label','Categoria '+g[0]);b.addEventListener('click',()=>{groupIndex=i;selected=0;render();});document.getElementById('categories').append(b);});
function setOpen(open){shell.classList.toggle('is-open',open);shell.inert=!open;shell.setAttribute('aria-hidden',String(!open));}
window.EmotesSP.setOpen=setOpen;
window.EmotesSP.notice=function(message){const toast=document.getElementById('toast');toast.textContent=message;toast.classList.add('visible');};
document.getElementById('center').addEventListener('click',()=>window.EmotesSP.send({action:'stop'}));
document.addEventListener('keydown',e=>{if(e.repeat)return;if(['KeyK','Escape'].includes(e.code)){e.preventDefault();window.EmotesSP.send({action:'close'});}});
document.addEventListener('contextmenu',e=>{e.preventDefault();window.EmotesSP.send({action:'close'});});render();setOpen(false);
