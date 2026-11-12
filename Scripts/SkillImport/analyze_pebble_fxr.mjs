/** Read-only FXR evidence/translation gate. No generated substitute particles.
 * Requires @cccode/fxr 32.1.3 unpacked in Saved/SkillImport/FxrTools/package.
 * Run from the repository root: node Scripts/SkillImport/analyze_pebble_fxr.mjs
 */
import fs from 'node:fs/promises';
import path from 'node:path';
import {createHash} from 'node:crypto';
import {pathToFileURL,fileURLToPath} from 'node:url';
import assert from 'node:assert/strict';
const repo=path.resolve(path.dirname(fileURLToPath(import.meta.url)),'../..');
const tool=path.join(repo,'Saved/SkillImport/FxrTools/package');
const {FXR,Game}=await import(pathToFileURL(path.join(tool,'dist/fxr.js')));
const version=JSON.parse(await fs.readFile(path.join(tool,'package.json'))).version;
assert.equal(version,'32.1.3','Review schema changes before accepting another decoder version');
const source=path.join(repo,'Saved/SkillImport/GlintstonePebble');
const output=path.join(source,'FxrAudit');await fs.mkdir(output,{recursive:true});
const hash=b=>createHash('sha256').update(b).digest('hex');
const roles={510010:'sigil',523000:'cast',523001:'release',523002:'flight',523003:'impact'};
const pending=Object.keys(roles).map(Number),seen=new Set(),effects=[];
while(pending.length){
 const id=pending.shift();if(seen.has(id))continue;seen.add(id);
 const filename=`f${String(id).padStart(9,'0')}.fxr`,bytes=await fs.readFile(path.join(source,filename));
 const fx=FXR.read(bytes,Game.EldenRing);assert.equal(fx.id,id);
 const semantic=fx.serialize();
 // This checks codec structural preservation, not equivalence of UE rendering.
 const roundtrip=FXR.read(fx.toArrayBuffer(Game.EldenRing),Game.EldenRing).serialize();
 assert.deepEqual(roundtrip,semantic,`Semantic roundtrip mismatch: ${id}`);
 await fs.writeFile(path.join(output,`${id}.json`),JSON.stringify(semantic,null,2));
 const nodes=[],refs=[],appearances={},curves={},modifiers={};let unknownFields=0;
 function inspect(v){
  if(!v||typeof v!=='object')return;
  if(v.function)curves[v.function]=(curves[v.function]??0)+1;
  if(Array.isArray(v.modifiers))for(const m of v.modifiers)modifiers[m.type]=(modifiers[m.type]??0)+1;
  for(const [k,x]of Object.entries(v)){if(k.startsWith('unk'))unknownFields++;inspect(x);}
 }
 function walk(n,where='root'){
  if(n.sfx){refs.push({path:where,sfx:n.sfx});pending.push(n.sfx);}
  for(const [i,c]of(n.configs??[]).entries()){
   const a=c.appearance,appearance=a?.serialize()??null;
   const name=appearance?a.constructor.name:null;
   if(name)appearances[name]=(appearances[name]??0)+1;
   nodes.push({path:`${where}/configs/${i}`,nodeType:n.type,
    stateConfigMap:n.stateConfigMap,attributes:c.nodeAttributes?.serialize(),
    emitter:c.emitter?.serialize(),particleAttributes:c.particleAttributes?.serialize(),
    renderer:name,appearance,translationStatus:name?'unimplemented':'nonvisual_or_container'});
  }
  for(const [i,child]of(n.nodes??[]).entries())walk(child,`${where}/nodes/${i}`);
 }
 walk(fx.root);inspect(semantic);
 const list=await fs.readFile(path.join(source,filename.replace('.fxr','.ffxreslist')),'utf8');
 const resources=[];
 for(const entry of list.split(/\r?\n/).map(x=>x.trim()).filter(Boolean)){
  const local=path.basename(entry).replace(/\.tif$/i,'.dds').replace(/\.sib$/i,'.flver');
  const content=await fs.readFile(path.join(source,local));
  resources.push({entry,local,sha256:hash(content)});
 }
 effects.push({id,role:roles[id]??'nested',filename,sha256:hash(bytes),
   semanticRoundtrip:true,refs,resources,nodeConfigs:nodes.length,appearances,curves,modifiers,
   unknownFieldOccurrences:unknownFields,nodes});
}
assert.equal(effects.find(e=>e.id===523001).nodes.filter(n=>n.renderer).length,0,
 'The verified release FXR has no visual renderer; do not add an invented release flash');
const sigil=effects.find(e=>e.id===510010);
assert.equal(sigil.nodes.filter(n=>n.appearance?.layer1===60130||n.appearance?.layer2===60130).length,2);
assert(seen.has(430)&&seen.has(440),'Missing nested FXRs');
const report={decoder:{name:'@cccode/fxr',version,source:'https://github.com/EvenTorset/fxr',
 sha256:hash(await fs.readFile(path.join(tool,'dist/fxr.js')))},
 status:'source_audited_not_reconstructed',uePromotionAllowed:false,
 blockers:['No validated UE renderer mapping for the audited FXR primitives/material equations',
 'TAE bank 37 and behavior-driven start/loop/release transitions are not verified',
 'Source dummy-poly/staff transform and target hand attachment are not equivalent',
 'Source/target pose alignment and full-body/upper-body composition need visual acceptance',
 'No synchronized original-game motion/VFX reference for multi-frame acceptance'],effects};
await fs.writeFile(path.join(output,'manifest.json'),JSON.stringify(report,null,2));
console.log(JSON.stringify(effects.map(({id,role,nodeConfigs,appearances,curves,refs})=>({id,role,nodeConfigs,appearances,curves,refs})),null,2));
console.log('FXR_SOURCE_AUDIT_PASSED; UE_PROMOTION_BLOCKED');
