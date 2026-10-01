/* Native Meridian transport for Emotes SP. GPL-3.0-or-later. */
(function(){
  'use strict';
  let attached=false;
  window.EmotesSP={
    send(message){
      if(!attached || typeof window.emotesSPMessage!=='function') return;
      window.emotesSPMessage(JSON.stringify(message));
    },
    attach(){
      if(attached) return true;
      if(typeof window.emotesSPMessage!=='function' || typeof this.setOpen!=='function') return false;
      attached=true;
      this.send({action:'ready'});
      return true;
    }
  };
  window.addEventListener('error',()=>window.EmotesSP.send({action:'ui-error'}));
})();
