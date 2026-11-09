// D-Sprite updates walking/eating every 800ms and its wash wave every 100ms.
export const CARE_TIMING={walk:800,eatStep:800,eatSteps:8,washStep:100,washSteps:24};
export function foodStage(step){return step>=7?3:step>=5?2:step>=3?1:0;}
export function actionStep(start,now,interval){return Math.max(0,Math.floor((now-start)/interval));}
