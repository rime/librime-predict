# librime-predict
librime plugin. predict next word.

## Usage
* Put the db file (by default `predict.db`) in rime user directory.
* In `*.schema.yaml`, add `predictor` to the list of `engine/processors` before `key_binder`,
or patch the schema with: `engine/processors/@before 0: predictor`
* Config items for your predictor:
```yaml
predictor:
  # predict db file in user directory/shared directory
  # default to 'predict.db'
  db: predict.db
  # max prediction candidates every time
  # default to 0, which means showing all candidates
  # you may set it the same with page_size so that period doesn't trigger next page
  max_candidates: 5
  # max continuous prediction times
  # default to 0, which means no limitation
  max_iterations: 1
```
* (Optional) Add a switch to toggle prediction:
```yaml
switches:
  - name: prediction
    states: [ 关闭预测, 开启预测 ]
    reset: 1
```
* Deploy and enjoy.
