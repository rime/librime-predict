use anyhow::Result;
use log::{debug, info};
use std::collections::BTreeMap;
use std::fs::File;
use std::io::{self, BufRead, BufReader, Write};
use std::iter::FromIterator;
use std::path::PathBuf;
use structopt::StructOpt;

type PredictData = BTreeMap<String, Vec<(String, u32)>>;

fn add_record(data: &mut PredictData, key: &str, value: &str, weight: u32) {
    if let Some(ref mut entry) = data.get_mut(key) {
        if let Some(ref mut r) = entry.iter_mut().find(|r| r.0 == value) {
            if r.1 < weight {
                r.1 = weight;
                debug!("updated {} : {} = {}", key, value, weight);
            }
        } else {
            entry.push((value.to_string(), weight));
            debug!("added {} : {} = {}", key, value, weight);
        }
    } else {
        data.insert(key.to_string(), vec![(value.to_string(), weight)]);
        debug!("added key {} with {} = {}", key, value, weight);
    }
}

#[derive(Debug, StructOpt)]
struct Cli {
    #[structopt(long)]
    filter_weight: Option<u32>,
    #[structopt(long)]
    max_candidates: Option<usize>,
    #[structopt(parse(from_os_str))]
    input_files: Vec<PathBuf>,
}

fn main() -> Result<()> {
    env_logger::init();
    let args = Cli::from_args();
    let mut data = PredictData::new();
    for file_path in args.input_files {
        info!("reading file {:?}", file_path);
        let file = File::open(file_path)?;
        let reader = BufReader::new(file);
        for line in reader.lines() {
            if let Ok(line) = line {
                let values = line.split('\t').collect::<Vec<&str>>();
                let weight = values[1].parse::<u32>()?;
                debug!("weight = {:?}", weight);
                if let Some(min_weight) = args.filter_weight {
                    if weight < min_weight {
                        continue;
                    }
                }
                assert!(values.len() == 2);
                let key = &values[0];
                debug!("key = {:?}", key);
                if key.contains(' ') {
                    let ngram = key.split(' ').collect::<Vec<&str>>();
                    assert!(ngram.len() == 2);
                    if ngram[1] == "$" {
                        continue;
                    }
                    add_record(&mut data, ngram[0], ngram[1], weight);
                } else {
                    let chars = key.chars().collect::<Vec<_>>();
                    for i in 1..chars.len() {
                        let key = String::from_iter(&chars[0..i]);
                        let value = String::from_iter(&chars[i..]);
                        debug!("key = {}, value = {}, split at {}", key, value, i);
                        add_record(&mut data, &key, &value, weight);
                    }
                }
            }
        }
    }
    let stdout = io::stdout();
    let mut out = stdout.lock();
    for (key, entry) in data.iter_mut() {
        // sort by weight descending.
        entry.sort_by(|a, b| b.1.cmp(&a.1));
        // keep the specified number of top candidates.
        if let Some(max_len) = args.max_candidates {
            if entry.len() > max_len {
                entry.resize_with(max_len, Default::default);
            }
        }
        for record in entry {
            writeln!(out, "{}\t{}\t{}", key, record.0, record.1)?;
        }
    }
    Ok(())
}
