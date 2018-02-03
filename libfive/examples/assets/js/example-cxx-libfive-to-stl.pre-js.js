
function renderExportedSTL() {

  var scene = new THREE.Scene();
  scene.background = new THREE.Color(0x404040);

  var camera = new THREE.PerspectiveCamera( 75, window.innerWidth/window.innerHeight, 0.1, 1000 );
  camera.position.z = 5;

  var renderer = new THREE.WebGLRenderer();
  renderer.setSize( window.innerWidth/2, window.innerHeight/2 );
  document.body.prepend( renderer.domElement );

  var geometry = new THREE.BoxGeometry( 1, 1, 1 );
  var material = new THREE.MeshStandardMaterial( { color: 0x00ff00, dithering: true, flatShading: true } );
  var cube = new THREE.Mesh( geometry, material );
  scene.add( cube );

  var light = new THREE.AmbientLight( 0xA0A0A0 );
  scene.add( light );

  var spotLight = new THREE.SpotLight( 0xffffff );
  spotLight.position.set( 100, 100, 100 );
  scene.add( spotLight );

  var light = new THREE.PointLight( 0xff0000, 1, 100 );
  light.position.set( 2, 2, 2 );
  scene.add( light );

  var animate = function () {
    requestAnimationFrame( animate );

    cube.rotation.x += 0.01;
    cube.rotation.y += 0.01;

    spotLight.position.x = Math.sin(Date.now()/1000) * 100;

    renderer.render(scene, camera);
  };

  animate();

  var loadModel = function(filepath) {
    var loader = new THREE.STLLoader();
    var file_content = FS.readFile(filepath);
    var result = loader.parse(file_content.buffer);

    var material = new THREE.MeshStandardMaterial( { color: 0x0000ff, dithering: true, flatShading: true } );

    var new_model = scene.add( new THREE.Mesh(result, material))

  };

  loadModel("/exported.stl");

};


Module['postRun'].push(renderExportedSTL);

