
BLENDER=blender

all : ../dist/city.pnct ../dist/city.scene ../dist/redcar.pnct ../dist/redcar.scene ../dist/bluecar.pnct ../dist/bluecar.scene ../dist/grenade.pnct ../dist/grenade.scene

../dist/city.pnct : city.blend export-meshes.py
	$(BLENDER) --background --python export-meshes.py -- city.blend:Scene '$@'
../dist/city.scene : city.blend export-scene.py
	$(BLENDER) --background --python export-scene.py -- city.blend:Scene '$@'
../dist/redcar.pnct : car.blend export-meshes.py
	$(BLENDER) --background --python export-meshes.py -- car.blend:red '$@'
../dist/redcar.scene : car.blend export-scene.py
	$(BLENDER) --background --python export-scene.py -- car.blend:red '$@'
../dist/bluecar.pnct : car.blend export-meshes.py
	$(BLENDER) --background --python export-meshes.py -- car.blend:blue '$@'
../dist/bluecar.scene : car.blend export-scene.py
	$(BLENDER) --background --python export-scene.py -- car.blend:blue '$@'
../dist/grenade.pnct : grenade.blend export-meshes.py
	$(BLENDER) --background --python export-meshes.py -- grenade.blend '$@'
../dist/grenade.scene : grenade.blend export-scene.py
	$(BLENDER) --background --python export-scene.py -- grenade.blend '$@'
